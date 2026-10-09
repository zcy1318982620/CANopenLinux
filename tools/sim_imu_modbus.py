#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
虚拟 IMU：Modbus RTU 从站仿真（纯标准库，不用 pymodbus）。

用途：本机没有 IMU 硬件时，给 C 语言 Modbus RTU 主站（agv_modbus.c）当对端，
      把 CRC / 切帧 / 超时 / 异常码 / float32 字序 全部先调通——和本项目
      "用 vcan 仿真电机从站" 是同一套思路（见 docs/MODBUS_LEARNING_MAP_print.html §10）。

寄存器表（与学习册 §10.1 一致，0 基地址）：
  输入寄存器 0x04（只读，18 个 = 9×float32）：
    0x0000~0x0005  姿态角 roll/pitch/yaw (rad)
    0x0006~0x000B  角速度 gyro x/y/z (rad/s)
    0x000C~0x0011  加速度 acc x/y/z (m/s^2)
  保持寄存器 0x06（读写）：
    0x0100  零偏校准命令（写 1 触发）
    0x0101  输出速率 (Hz)

用法：
  # 自建一对虚拟串口，打印从站侧路径给 C 主站打开
  python3 tools/sim_imu_modbus.py pty
  # 输出：SLAVE=/dev/pts/3  （另一窗口：./mb_probe /dev/pts/3）

  # 接真串口（硬件到位后）
  python3 tools/sim_imu_modbus.py /dev/ttyUSB0 --baud 115200 --addr 1

  # 故障注入（验证主站异常/重试/超时分支）
  python3 tools/sim_imu_modbus.py pty --inject-exception 2   # 一律回异常码 0x02
  python3 tools/sim_imu_modbus.py pty --inject-bad-crc        # CRC 故意写错
  python3 tools/sim_imu_modbus.py pty --inject-drop 3         # 前 3 帧不响应(超时)

  # 字序联调（对照主站 mb_cfg_t.word_order）
  python3 tools/sim_imu_modbus.py pty --word-order CDAB
"""

import argparse
import math
import os
import select
import struct
import sys
import termios
import time
import tty

# 默认寄存器初值（与 mb_probe 的期望值保持一致，便于端到端判 PASS）
DEF_POSE = (0.100, -0.200, 1.500)          # roll, pitch, yaw (rad)
DEF_GYRO = (0.010, 0.020, 0.030)           # rad/s
DEF_ACC = (0.000, 0.000, 9.810)            # m/s^2

FC_READ_HOLDING = 0x03
FC_READ_INPUT = 0x04
FC_WRITE_SINGLE = 0x06
FC_WRITE_MULTI = 0x10

EXC_ILLEGAL_FUNC = 0x01
EXC_ILLEGAL_ADDR = 0x02
EXC_ILLEGAL_VALUE = 0x03


def crc16(data: bytes) -> int:
    """CRC16-Modbus：多项式 0xA001、初值 0xFFFF（低字节先发）。"""
    crc = 0xFFFF
    for byte in data:
        crc ^= byte
        for _ in range(8):
            if crc & 1:
                crc = (crc >> 1) ^ 0xA001
            else:
                crc >>= 1
    return crc & 0xFFFF


def append_crc(pdu: bytes) -> bytes:
    c = crc16(pdu)
    return pdu + bytes((c & 0xFF, c >> 8))


def enc_float(f: float, order: str):
    """float32 → 两个 16 位寄存器，按指定字序（与 agv_modbus.c 的四种对应）。"""
    b = struct.pack(">f", f)               # 内存序 A B C D
    a_, b_, c_, d_ = b[0], b[1], b[2], b[3]
    if order == "ABCD":
        r0, r1 = (a_ << 8) | b_, (c_ << 8) | d_
    elif order == "CDAB":
        r0, r1 = (c_ << 8) | d_, (a_ << 8) | b_
    elif order == "BADC":
        r0, r1 = (b_ << 8) | a_, (d_ << 8) | c_
    elif order == "DCBA":
        r0, r1 = (d_ << 8) | c_, (b_ << 8) | a_
    else:
        raise ValueError("word-order 必须是 ABCD/CDAB/BADC/DCBA")
    return r0, r1


class Slave:
    def __init__(self, args):
        self.addr = args.addr
        self.order = args.word_order
        self.animate = args.animate
        # 输入寄存器：静态初值，可选动画在 yaw/gyro 上叠加正弦
        self.io_src = {"pose": DEF_POSE, "gyro": DEF_GYRO, "acc": DEF_ACC}
        # 保持寄存器
        self.holding = {0x0100: 0, 0x0101: 100}
        # 故障注入
        self.inject_exc = args.inject_exception
        self.inject_bad_crc = args.inject_bad_crc
        self.inject_drop = args.inject_drop
        self.t0 = time.time()

    # ---- 输入寄存器视图（每次请求现算，支持动画）----
    def input_regs(self):
        pose, gyro, acc = self.io_src["pose"], self.io_src["gyro"], self.io_src["acc"]
        if self.animate:
            t = time.time() - self.t0
            pose = (pose[0], pose[1], pose[2] * math.sin(2 * math.pi * t))
            gyro = (gyro[0], gyro[1], gyro[2] * math.cos(2 * math.pi * t))
        vals = []
        for f in pose + gyro + acc:
            vals.extend(enc_float(f, self.order))
        return vals

    # ---- 保持寄存器读视图：0x0100/0x0101 之外的地址视为不存在 ----
    def holding_read(self, reg, qty):
        if reg in self.holding and qty == 1:
            return [self.holding[reg]]
        return None

    # ---- 出帧 ----
    def _send(self, out_fd, frame: bytes):
        if self.inject_drop > 0:
            self.inject_drop -= 1
            sys.stderr.write("[sim] 故障注入：丢弃一帧响应(模拟超时)\n")
            return
        if self.inject_bad_crc:
            frame = frame[:-1] + bytes((frame[-1] ^ 0xFF,))
        os.write(out_fd, frame)

    @staticmethod
    def _exc(addr, func, code) -> bytes:
        return append_crc(bytes((addr, func | 0x80, code)))

    # ---- 处理一帧请求 ----
    def handle(self, out_fd, frame: bytes):
        # 最短 ADU：地址+功能码+CRC(2) = 4
        if len(frame) < 4:
            sys.stderr.write("[sim] 丢弃过短帧 %s\n" % frame.hex(" "))
            return
        if crc16(frame) != 0:
            sys.stderr.write("[sim] CRC 错，丢弃：%s\n" % frame.hex(" "))
            return

        addr, func = frame[0], frame[1]
        if addr not in (self.addr, 0):     # 非本从站（0 为广播）→ 静默
            return

        if self.inject_exc is not None:
            sys.stderr.write("[sim] 故障注入：返回异常码 0x%02X\n" % self.inject_exc)
            self._send(out_fd, self._exc(addr, func, self.inject_exc))
            return

        if func in (FC_READ_HOLDING, FC_READ_INPUT):
            self._handle_read(out_fd, frame, addr, func)
        elif func == FC_WRITE_SINGLE:
            self._handle_write_single(out_fd, frame, addr)
        elif func == FC_WRITE_MULTI:
            self._handle_write_multi(out_fd, frame, addr)
        else:
            sys.stderr.write("[sim] 非法功能码 0x%02X\n" % func)
            self._send(out_fd, self._exc(addr, func, EXC_ILLEGAL_FUNC))

    def _handle_read(self, out_fd, frame, addr, func):
        reg = (frame[2] << 8) | frame[3]
        qty = (frame[4] << 8) | frame[5]
        if qty < 1 or qty > 125:
            self._send(out_fd, self._exc(addr, func, EXC_ILLEGAL_VALUE))
            return

        if func == FC_READ_INPUT:
            vals = self.input_regs()
            if reg + qty > len(vals):
                self._send(out_fd, self._exc(addr, func, EXC_ILLEGAL_ADDR))
                return
            data = vals[reg:reg + qty]
        else:                                   # 0x03 保持寄存器
            data = self.holding_read(reg, qty)
            if data is None:
                self._send(out_fd, self._exc(addr, func, EXC_ILLEGAL_ADDR))
                return

        payload = bytearray((addr, func, qty * 2))
        for v in data:
            payload += bytes((v >> 8, v & 0xFF))
        self._send(out_fd, append_crc(bytes(payload)))

    def _handle_write_single(self, out_fd, frame, addr):
        reg = (frame[2] << 8) | frame[3]
        val = (frame[4] << 8) | frame[5]
        if reg not in self.holding:
            self._send(out_fd, self._exc(addr, FC_WRITE_SINGLE, EXC_ILLEGAL_ADDR))
            return
        if reg == 0x0100 and val == 1:
            sys.stderr.write("[sim] 收到零偏校准命令\n")
        self.holding[reg] = val
        self._send(out_fd, append_crc(frame[:6]))   # 0x06 原样回显
        sys.stderr.write("[sim] 写单个 0x%04X = %d\n" % (reg, val))

    def _handle_write_multi(self, out_fd, frame, addr):
        reg = (frame[2] << 8) | frame[3]
        qty = (frame[4] << 8) | frame[5]
        bc = frame[6]
        if bc != qty * 2 or len(frame) != 9 + bc:
            self._send(out_fd, self._exc(addr, FC_WRITE_MULTI, EXC_ILLEGAL_VALUE))
            return
        for i in range(qty):
            a = reg + i
            if a not in self.holding:
                self._send(out_fd, self._exc(addr, FC_WRITE_MULTI, EXC_ILLEGAL_ADDR))
                return
        for i in range(qty):
            self.holding[reg + i] = (frame[7 + 2 * i] << 8) | frame[8 + 2 * i]
        self._send(out_fd, append_crc(frame[:6]))   # 回显 起始地址+数量
        sys.stderr.write("[sim] 写多个 0x%04X x%d\n" % (reg, qty))


def open_port(path, baud):
    """返回 (io_fd, slave_path)。path=='pty' 时自建 pty 对。"""
    if path == "pty":
        master_fd, slave_fd = os.openpty()
        slave_path = os.ttyname(slave_fd)
        tty.setraw(slave_fd)                       # 从站侧设原始模式，保证字节透明
        # 保持 slave_fd 打开以维持 pts 存活；IO 走 master_fd
        return master_fd, slave_path, slave_fd
    fd = os.open(path, os.O_RDWR | os.O_NOCTTY | os.O_NONBLOCK)
    # 真串口：设 8N1 原始模式
    attrs = termios.tcgetattr(fd)
    iflag, oflag, cflag, lflag, _ispeed, _ospeed, cc = attrs
    cflag = (cflag & ~termios.CSIZE) | termios.CS8
    cflag &= ~(termios.PARENB | termios.CSTOPB)
    cflag |= (termios.CLOCAL | termios.CREAD)
    lflag = 0
    iflag = 0
    oflag = 0
    cc = list(cc)
    cc[termios.VMIN] = 0
    cc[termios.VTIME] = 0
    speed = {9600: termios.B9600, 19200: termios.B19200, 38400: termios.B38400,
             57600: termios.B57600, 115200: termios.B115200, 230400: termios.B230400}[baud]
    termios.tcsetattr(fd, termios.TCSANOW,
                      [iflag, oflag, cflag, lflag, speed, speed, cc])
    termios.tcflush(fd, termios.TCIOFLUSH)
    return fd, path, None


def main():
    ap = argparse.ArgumentParser(description="虚拟 IMU Modbus RTU 从站（纯标准库）")
    ap.add_argument("port", help="串口设备路径，或 'pty' 自建虚拟串口对")
    ap.add_argument("--addr", type=int, default=1, help="从站地址(默认 1)")
    ap.add_argument("--baud", type=int, default=115200, help="波特率(默认 115200)")
    ap.add_argument("--word-order", default="ABCD",
                    choices=["ABCD", "CDAB", "BADC", "DCBA"], help="float32 字序")
    ap.add_argument("--animate", action="store_true", help="姿态/角速度按正弦动画")
    ap.add_argument("--inject-exception", type=int, default=None, metavar="CODE",
                    help="对所有请求返回该异常码(如 2)")
    ap.add_argument("--inject-bad-crc", action="store_true", help="响应 CRC 故意写错")
    ap.add_argument("--inject-drop", type=int, default=0, metavar="N",
                    help="前 N 帧不响应(模拟超时)")
    args = ap.parse_args()

    if not 1 <= args.addr <= 247:
        sys.exit("从站地址须在 1..247")

    io_fd, slave_path, keep_fd = open_port(args.port, args.baud)
    slave = Slave(args)

    # C 主站需要这个路径来 open()；打印到 stdout 并 flush
    print("SLAVE=%s" % slave_path, flush=True)
    sys.stderr.write("[sim] 虚拟 IMU 从站就绪：addr=%d baud=%d word_order=%s port=%s\n"
                     % (args.addr, args.baud, args.word_order, slave_path))

    buf = bytearray()
    try:
        while True:
            r, _, _ = select.select([io_fd], [], [], 0.02)
            if r:
                try:
                    data = os.read(io_fd, 256)
                except OSError:
                    break
                if not data:
                    break
                buf += data
            elif buf:
                # 总线静默 > t3.5：视为一帧结束
                slave.handle(io_fd, bytes(buf))
                buf.clear()
    except KeyboardInterrupt:
        pass
    finally:
        if keep_fd is not None:
            os.close(keep_fd)
        os.close(io_fd)


if __name__ == "__main__":
    main()
