#!/usr/bin/env python3
"""
模拟松下 AGV 的左右两个电机驱动器，在虚拟/真实 CAN 总线上周期发送 RPDO 帧。

对应关系（来自本工程 CO_application.c / application/OD.c）：
    0x38A --> 右电机 RPDO1（映射 0x6041:00 状态字 + 0x6064:00 实际位置）
    0x38B --> 左电机 RPDO2（同上）

帧格式（DLC = 6，小端）：
    B0~B1 : 状态字 0x6041 (uint16)
    B2~B5 : 实际位置 0x6064 (int32)

用法：
    python3 tools/sim_motors.py vcan0            # 默认 node-id = 1
    python3 tools/sim_motors.py vcan0 4          # 指定 node-id = 4
    python3 tools/sim_motors.py vcan0 1 --static # 两轮都不动（仅验证链路）
    python3 tools/sim_motors.py vcan0 1 --both   # 两轮同速直行（验证左右两路都通）

不依赖任何第三方库（只用标准库 socket / struct）。
"""
import socket
import struct
import sys
import time

iface = sys.argv[1] if len(sys.argv) > 1 else "vcan0"
node_id = int(sys.argv[2]) if len(sys.argv) > 2 else 1
static = "--static" in sys.argv
both = "--both" in sys.argv

# --- 打开 Raw CAN socket ---
try:
    s = socket.socket(socket.AF_CAN, socket.SOCK_RAW, socket.CAN_RAW)
    s.bind((iface,))
except OSError as e:
    print(f"[错误] 打不开 CAN 接口 {iface}: {e}")
    print("请先确认虚拟总线已建立：")
    print("    sudo modprobe vcan")
    print(f"    sudo ip link add dev {iface} type vcan")
    print(f"    sudo ip link set up {iface}")
    sys.exit(1)


def send(can_id, data):
    """发送一帧标准 CAN（can_frame 结构：4B id + 1B dlc + 3B pad + 8B data）。"""
    s.send(struct.pack("=IB3x8s", can_id, len(data), data))


# --- 先发 NMT Start，让设备从 PRE-OPERATIONAL 进入 OPERATIONAL ---
# NMT 命令帧：COB-ID = 0x000，数据 = [命令, 节点号]；0x01 = Start remote node
send(0x000, bytes([0x01, node_id]))
print(f"[NMT] 已向 node {node_id} 发送 Start 命令（进入 OPERATIONAL）")

STATUSWORD = 0x0237  # 常见 CiA402 状态字：Operation enabled
PERIOD = 0.1         # 10 Hz

print(f"[SIM] 开始向 {iface} 发送  0x38A(右轮) / 0x38B(左轮)  DLC=6，10Hz")
if static:
    print("[SIM] 两轮均不动（静态），仅验证 RPDO→OD 链路。Ctrl+C 退出。")
elif both:
    print("[SIM] 两轮同速(1000 计数/秒)直行，用于验证左右两路 RPDO 都生效。Ctrl+C 退出。")
else:
    print("[SIM] 右轮每秒 1000 计数、左轮不动（模拟绕左轮转）。Ctrl+C 退出。")

t0 = time.time()
try:
    while True:
        if static:
            pos_r = 0
            pos_l = 0
        elif both:
            t = time.time() - t0
            pos_r = int(1000 * t)  # 两轮同速，车直行
            pos_l = int(1000 * t)
        else:
            t = time.time() - t0
            pos_r = int(1000 * t)  # 每秒 1000 计数
            pos_l = 0
        frame = struct.pack("<Hi", STATUSWORD, pos_r)  # 2B 状态字 + 4B 位置 = 6B
        send(0x38A, frame)
        send(0x38B, struct.pack("<Hi", STATUSWORD, pos_l))
        time.sleep(PERIOD)
except KeyboardInterrupt:
    print("\n[SIM] 已停止。")
