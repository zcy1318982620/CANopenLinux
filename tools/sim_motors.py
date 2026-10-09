#!/usr/bin/env python3
"""
模拟松下 AGV 的左右两个电机驱动器，在虚拟/真实 CAN 总线上周期发送 RPDO 帧。

对应关系（来自本工程 CO_application.c / application/OD.c）：
    0x38A --> 右电机 RPDO1（映射 0x6041:00 状态字 + 0x6064:00 实际位置）
    0x38B --> 左电机 RPDO2（同上）
    0x70A --> 右电机心跳（node 10，载荷 = NMT 状态字节 0x05=OPERATIONAL）
    0x70B --> 左电机心跳（node 11，同上）

帧格式（DLC = 6，小端）：
    B0~B1 : 状态字 0x6041 (uint16)
    B2~B5 : 实际位置 0x6064 (int32)

用法：
    python3 tools/sim_motors.py vcan0            # 默认 node-id = 1
    python3 tools/sim_motors.py vcan0 4          # 指定 node-id = 4
    python3 tools/sim_motors.py vcan0 1 --static # 两轮都不动（仅验证链路）
    python3 tools/sim_motors.py vcan0 1 --both   # 两轮同速直行（验证左右两路都通）
    python3 tools/sim_motors.py vcan0 --drop-hb-r # 停发右轮心跳（自开机即停）
    python3 tools/sim_motors.py vcan0 --drop-hb-l # 停发左轮心跳（自开机即停）

故障注入开关（P2 §2.4，用于验证"外部乱来时扛不扛得住"）：
    python3 tools/sim_motors.py vcan0 --drop-hb-after 3 # 心跳正常发 3s 后停发(两轮)
                                                       # 用于触发"运行中掉线→超时降级"
    python3 tools/sim_motors.py vcan0 --bad-dlc       # RPDO 只发 3 字节(DLC 错误)
    python3 tools/sim_motors.py vcan0 --out-of-range  # 位置交替发 INT32_MAX/MIN(越界值)
    python3 tools/sim_motors.py vcan0 --flood         # 紧循环高频灌帧(压测丢帧)

不依赖任何第三方库（只用标准库 socket / struct）。
"""
import socket
import struct
import sys
import time

iface = sys.argv[1] if len(sys.argv) > 1 else "vcan0"
# node-id 仅在提供了数字参数时解析，避免把 --static 等开关误当节点号
node_id = int(sys.argv[2]) if len(sys.argv) > 2 and sys.argv[2].isdigit() else 1
static = "--static" in sys.argv
both = "--both" in sys.argv
drop_hb_r = "--drop-hb-r" in sys.argv   # 停发右轮心跳，模拟 node10 掉线
drop_hb_l = "--drop-hb-l" in sys.argv   # 停发左轮心跳，模拟 node11 掉线
# --drop-hb-after N：前 N 秒正常发心跳，之后两轮心跳一起停发。
# 与 --drop-hb-r/--drop-hb-l(自开机即停)不同：HBconsumer 必须先收到过心跳
# 才会进入 ACTIVE，只有从 ACTIVE 才会因超时进入 TIMEOUT，故验证"运行中掉线"
# 必须让心跳先到达、再断掉。
hb_drop_after = None
if "--drop-hb-after" in sys.argv:
    _i = sys.argv.index("--drop-hb-after")
    if _i + 1 < len(sys.argv):
        hb_drop_after = float(sys.argv[_i + 1])
bad_dlc = "--bad-dlc" in sys.argv           # RPDO 只发 3 字节(模拟 DLC 错误)
out_of_range = "--out-of-range" in sys.argv # 位置塞 INT32_MAX/MIN(越界值)
flood = "--flood" in sys.argv               # 紧循环高频灌帧(压测)

# 从站节点号（与 CO_application.c 的 DL_NODE_R / DL_NODE_L 保持一致）
NODE_R = 10
NODE_L = 11
HB_STATE = 0x05     # 心跳载荷 = NMT 状态：OPERATIONAL
INT32_MAX = 2**31 - 1
INT32_MIN = -(2**31)

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
print(f"[SIM] 心跳：0x70A(node{NODE_R}) {'停发' if drop_hb_r else '周期发'}，"
      f"0x70B(node{NODE_L}) {'停发' if drop_hb_l else '周期发'}"
      + (f"，{hb_drop_after:g}s 后两轮一起停发" if hb_drop_after is not None else ""))
if static:
    print("[SIM] 两轮均不动（静态），仅验证 RPDO→OD 链路。Ctrl+C 退出。")
elif both:
    print("[SIM] 两轮同速(1000 计数/秒)直行，用于验证左右两路 RPDO 都生效。Ctrl+C 退出。")
else:
    print("[SIM] 右轮每秒 1000 计数、左轮不动（模拟绕左轮转）。Ctrl+C 退出。")

if bad_dlc:
    print("[SIM] !! 故障注入：0x38A/0x38B 只发 3 字节（DLC 错误），验证协议层校验。")
if out_of_range:
    print("[SIM] !! 故障注入：位置交替发 INT32_MAX/INT32_MIN（越界值）。")
if flood:
    print("[SIM] !! 故障注入：紧循环高频灌帧（无 sleep），压测 RX 队列丢帧。")

t0 = time.time()
tick = 0
try:
    while True:
        t = time.time() - t0
        if static:
            pos_r = 0
            pos_l = 0
        elif both:
            pos_r = int(1000 * t)  # 两轮同速，车直行
            pos_l = int(1000 * t)
        else:
            pos_r = int(1000 * t)  # 每秒 1000 计数
            pos_l = 0

        if out_of_range:
            # 交替塞 INT32_MAX / INT32_MIN，验证越界值不会让里程计跑飞或崩溃
            pos_r = INT32_MAX if (tick % 2 == 0) else INT32_MIN
            pos_l = INT32_MIN if (tick % 2 == 0) else INT32_MAX

        frame_r = struct.pack("<Hi", STATUSWORD, pos_r)  # 2B 状态字 + 4B 位置 = 6B
        frame_l = struct.pack("<Hi", STATUSWORD, pos_l)
        if bad_dlc:
            frame_r = frame_r[:3]  # 故意只发 3 字节，模拟 DLC 错误
            frame_l = frame_l[:3]
        send(0x38A, frame_r)
        send(0x38B, frame_l)

        # 周期心跳：COB-ID = 0x700 + nodeId，载荷 = NMT 状态（OPERATIONAL）
        # hb_drop_after 时刻之后两轮心跳统一停发（模拟运行中掉线）
        hb_on = (hb_drop_after is None) or (t < hb_drop_after)
        if not drop_hb_r and hb_on:
            send(0x700 + NODE_R, bytes([HB_STATE]))
        if not drop_hb_l and hb_on:
            send(0x700 + NODE_L, bytes([HB_STATE]))

        tick += 1
        if not flood:
            time.sleep(PERIOD)
except KeyboardInterrupt:
    print("\n[SIM] 已停止。")
