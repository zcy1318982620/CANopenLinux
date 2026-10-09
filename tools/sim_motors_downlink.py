#!/usr/bin/env python3
"""
下行控制(CiA402 使能时序)联调仿真：模拟左右两个电机从站。

与 sim_motors.py 的区别：sim_motors.py 是"单向"仿真——只会按固定曲线回发
状态字/位置；本脚本是"闭环从站"——会真正收下 AGV(主站)经 TPDO 下发的控制字
0x6040，跑一个迷你 CiA402 状态机，再把对应的状态字 0x6041 回发，从而能验证
AGV 侧的使能时序是否把从站一步步推进到 Operation enabled。

收发对应关系(来自本工程 CO_application.c / application/OD.c)：
    主站→从站 TPDO：0x50A(右轮) / 0x50B(左轮)，映射 0x6040:00 控制字 + 0x60FF:00 目标速度
    从站→主站 RPDO：0x38A(右轮) / 0x38B(左轮)，映射 0x6041:00 状态字 + 0x6064:00 实际位置

TPDO 帧格式(DLC = 6，小端)：
    B0~B1 : 控制字 0x6040 (uint16)
    B2~B5 : 目标速度 0x60FF (int32)
RPDO 帧格式(DLC = 6，小端)：
    B0~B1 : 状态字 0x6041 (uint16)
    B2~B5 : 实际位置 0x6064 (int32)

用法：
    python3 tools/sim_motors_downlink.py vcan0
    python3 tools/sim_motors_downlink.py can0 [--verbose]

不依赖任何第三方库(只用标准库 socket / struct)。
"""
import socket
import struct
import sys
import time

iface = sys.argv[1] if len(sys.argv) > 1 else "vcan0"
verbose = "--verbose" in sys.argv

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
    """发送一帧标准 CAN(can_frame 结构：4B id + 1B dlc + 3B pad + 8B data)。"""
    s.send(struct.pack("=IB3x8s", can_id, len(data), data))


# ===================== 迷你 CiA402 从站状态机 =====================
# 状态编码(与分册 S10 表 10-2 一致)
S_NOT_READY       = 0
S_SWITCH_ON_DISABLED = 1
S_READY_TO_SWITCH_ON = 2
S_SWITCHED_ON     = 3
S_OP_ENABLED      = 4
S_FAULT           = 5

# 各状态对应的状态字(0x6041)：低 4~6 位是关键，另带上 bit4(电压使能)+bit9(远程)
SW = {
    S_NOT_READY:          0x0210,  # xxxx xxxx x0xx 0000
    S_SWITCH_ON_DISABLED: 0x0250,  # xxxx xxxx x1xx 0000
    S_READY_TO_SWITCH_ON: 0x0231,  # xxxx xxxx x01x 0001
    S_SWITCHED_ON:        0x0233,  # xxxx xxxx x01x 0011
    S_OP_ENABLED:         0x0237,  # xxxx xxxx x01x 0111
    S_FAULT:              0x0218,  # xxxx xxxx x0xx 1000
}

NAME = {
    S_NOT_READY: "Not_ready", S_SWITCH_ON_DISABLED: "Switch_on_disabled",
    S_READY_TO_SWITCH_ON: "Ready_to_switch_on", S_SWITCHED_ON: "Switched_on",
    S_OP_ENABLED: "Operation_enabled", S_FAULT: "Fault",
}


class Motor:
    """一个从站电机的迷你 CiA402 状态机。"""

    def __init__(self, name):
        self.name = name
        self.state = S_SWITCH_ON_DISABLED   # 上电默认
        self.cw = 0
        self.target_vel = 0
        self.pos = 0                        # 累计位置(计数)，本仿真恒为 0(不转)

    def apply_controlword(self, cw, target_vel):
        self.cw = cw
        self.target_vel = target_vel

        # ① Fault：只有 bit7(Fault reset) 能复位
        if self.state == S_FAULT:
            if cw & 0x0080:
                self._goto(S_SWITCH_ON_DISABLED)
            return

        # ② 撤电压(bit1=0) 从任何状态退回 Switch on disabled
        if (cw & 0x0002) == 0:
            if self.state != S_SWITCH_ON_DISABLED:
                self._goto(S_SWITCH_ON_DISABLED)
            return

        # ③ 各状态迁移(只实现主路径；Quick stop 等从简)
        if self.state == S_SWITCH_ON_DISABLED:
            # Shutdown: bit1&bit2 置位、bit0 清零
            if (cw & 0x0006) == 0x0006:
                self._goto(S_READY_TO_SWITCH_ON)
        elif self.state == S_READY_TO_SWITCH_ON:
            if (cw & 0x0006) == 0x0006 and (cw & 0x0001):
                self._goto(S_SWITCHED_ON)      # Switch on
        elif self.state == S_SWITCHED_ON:
            if (cw & 0x000F) == 0x000F:
                self._goto(S_OP_ENABLED)       # Enable operation
            elif (cw & 0x0006) == 0x0006 and (cw & 0x0001) == 0:
                self._goto(S_READY_TO_SWITCH_ON)
        elif self.state == S_OP_ENABLED:
            if (cw & 0x000F) == 0x000F:
                pass                            # 保持使能
            elif (cw & 0x0007) == 0x0007:
                self._goto(S_SWITCHED_ON)       # Disable operation
            elif (cw & 0x0006) == 0x0006 and (cw & 0x0001) == 0:
                self._goto(S_READY_TO_SWITCH_ON)

    def _goto(self, st):
        if st != self.state:
            if verbose:
                print(f"[{self.name}] {NAME[self.state]} -> {NAME[st]} "
                      f"(cw=0x{self.cw:04X})")
            self.state = st

    def statusword(self):
        return SW[self.state]


right = Motor("R")
left = Motor("L")

# 从站节点号(与 CO_application.c 的 DL_NODE_R/L 一致)，本仿真同时扮演两个从站
NODE_R, NODE_L = 10, 11
operational = False   # 收到 NMT Start 后置 true，才开始回发 RPDO

print(f"[SIM] 下行闭环从站仿真已启动：{iface}"
      f"(收 0x50A/0x50B 控制字，回 0x38A/0x38B 状态字)，Ctrl+C 退出。")

PERIOD = 0.05   # RPDO 回发 20 Hz，比主站 1ms 节拍慢，只为给出状态反馈
last_tx = 0.0
s.settimeout(0.02)

try:
    while True:
        # ---- 收：处理主站下发的 TPDO 与 NMT ----
        try:
            while True:
                frame, _ = s.recvfrom(16)
                can_id = struct.unpack("=I", frame[:4])[0] & 0x1FFFFFFF
                dlc = frame[4]
                data = frame[8:8 + dlc]

                if can_id == 0x000 and dlc >= 2:       # NMT 命令
                    cmd, node = data[0], data[1]
                    if cmd == 0x01 and node in (0x00, NODE_R, NODE_L):
                        operational = True
                        print(f"[SIM] 收到 NMT Start(node={node}) → OPERATIONAL")
                elif can_id == 0x50A and dlc >= 6:     # 右轮 TPDO
                    cw, vel = struct.unpack("<Hi", data[:6])
                    right.apply_controlword(cw, vel)
                elif can_id == 0x50B and dlc >= 6:     # 左轮 TPDO
                    cw, vel = struct.unpack("<Hi", data[:6])
                    left.apply_controlword(cw, vel)
        except socket.timeout:
            pass

        # ---- 发：周期回发 RPDO(状态字 + 位置) ----
        now = time.time()
        if operational and (now - last_tx) >= PERIOD:
            last_tx = now
            send(0x38A, struct.pack("<Hi", right.statusword(), right.pos))
            send(0x38B, struct.pack("<Hi", left.statusword(), left.pos))

except KeyboardInterrupt:
    print("\n[SIM] 已停止。")
