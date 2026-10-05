# 项目交接说明（换对话 / 换任务前必读）

> 更新时间：**2026-10-05**
> 目的：把「用户需求与硬约束 / 已确定的路线 / 正在干什么 / 进度到哪 / 下一步怎么写」完整交接。
> 接手的人（或 AI）：**先完整读完本文件，再动手**。
>
> **本次（2026-10-05）更新要点：**
> 1. **分册进度大幅推进**：S2~S8 分册 + 面试 QA 均**已落地**（旧版记的「S2 未开始」已过期，见 4.1 / 第五节）。
> 2. **新增用户硬要求**（见第一节 13~18 条）：**以函数为单位讲代码 / 必须标 `文件:行号` 且必须有流程图 /
>    变量"在哪出现就在那说明" / 不再使用 `AskUserQuestion`**。
> 3. **进入"等电机"阶段**：硬件尚未到货，已给出**不用电机就能推进的学习清单**（见第十二节）。
> 4. **硬件采购调研已完成**（型号 / 渠道 / 闲鱼候选 / 必确认项，见第十三节）。
>
> 旧版交接（2026-09-28）内容停留在「写第 2 站分册」；更早（2026-09-27）停留在「vcan 被 sudo 卡住」。
> 现在**总线已建好、程序已实跑、抓包证据已拿到、十站分册基本齐**。

---

## 一、用户是谁 / 他要什么（全部要求，逐条列全）

- 中文口语化表达的学习者，**新手**，明确要求「**边学习相关知识边做项目**」。
- 硬件：**i.MX6ULL（迅为 imx6ull_pro）开发板**，在手边，可烧录。
- 已有基础：Qt 5.12.8 交叉编译、Linux 字符设备驱动。
- 职业目标方向：**嵌入式工业控制 / AMR / AGV**。
- 核心诉求：要一个**有背景、能写进简历的项目**，不要「教程级玩具」，不要「工具包 / SDK」。

### 硬性约束（多轮强调，务必遵守）

1. **必须是"别人已经做好的现成工程"，不要自己从零写。**
2. **不要自己生成骨架，要去调现成的底座。**
3. **先能说清"这东西能实现什么、简历上能写什么"，再动手。**
4. **买硬件可以，但要便宜**（预算敏感）。
5. **不要研究电机算法**（不写 FOC、不做电流环整定）。
6. 最终要落到 **CANopen / AMR / AGV** 这条线上。

### 交付方式约定（重要，别违反）

7. **推进节奏：先出「学习地图」，再逐站动手。**
8. **分册产出，不要一次给全部。**（原话：「分开生成，不要一次性给出全部，一步一步的学」）
9. **输出要凝练。**（原话：「输出太长，继续凝练」）
10. **必须逐行讲清「代码作用」+「关键变量的作用」**，深度对齐用户此前提供的 AI 问答（SocketCAN 过滤器那份）。
    （原话：「下面第二站你要加上代码作用，关键变量的作用，参考我的提问，要达到这个深度与广度」）
11. **不要碰已完成站点的既有内容**（原话：「先别管第一站的东西了」）。
12. 文档全部是 **HTML 打印版**，用户会自己「打印 / 导出 PDF」；参考排版模板已给定（见第四节）。

### 新增硬要求（2026-10-05 追加，务必逐条遵守）

13. **讲代码必须以「函数」为单位**：每个函数——它是干什么的、**被谁调用**、**又调用了谁**；
    不重要的细节不讲（不要逐行念代码，要讲清函数在链路里的位置）。
14. **必须标注 `文件:行号`**（用可点击的 `file:///...` 链接），**并且每讲一段链路必须配流程图**。
15. **变量「在哪出现就在那说明」**：不要在文末单独堆一张变量表了事，要在它实际出现的位置就地解释。
16. **要讲清「整个工程的运行链路」**：从进程启动到报文进出的完整路径，而不是孤立地讲某个模块。
17. **不再使用 `AskUserQuestion` 工具**（用户曾跳过并触发限制提示）→ 改为**自行读取信息 + 用文字请用户确认**。
18. **回复必须用中文。**

### 已经排除掉的方案（别再回头推荐）

- ❌ 让用户自己写 CANopen 主站 → 被明确否定。
- ❌ CANopenNode / CANopenLinux 单独当项目 → 用户认为这是「工具包 / SDK」，不是项目。
- ❌ `imx6ull_serial2net_gateway` 起步 → 已证伪（串口转网口，与 CANopen 无关）。
- ❌ Robby（ROS 2 双轮差速）→ 用户只有 ROS 1 Noetic，且 i.MX6ULL 跑不动 ROS 2，放弃。
- ❌ diffbot / OpenAMRobot / ROMR / ros2_canopen / ros_canopen / KaCanOpen / adi_tmc_coe / espp-canopen
  / hex-motor → 各有硬伤（非 CANopen 底层 / 平台太重 / 未生产就绪），未采用。

---

## 二、已确定的技术路线（用户已拍板）

**底座：松下 Panasonic「実験用 AGV」工程（CANopenLinux 的 sample1 分支）**

- 仓库：<https://github.com/Panasonic-Advanced-Technology/CANopenLinux/tree/sample1>（Apache-2.0）
- 本质：**双轮差速 AGV 的 CANopen 设备（从站）工程**，纯 C + SocketCAN，**不需要 ROS** → 可跑 i.MX6ULL。
- 真实硬件背景：松下实验用 AGV，配 **2 个 Oriental Motor BLVD-KRD 驱动器**。
- 配套日文教程（原作者 yamamoto-kazusige，2023-08）：
  <https://qiita.com/yamamoto-kazusige/items/2e14a164184a2e67742b>
- **实施顺序：先在 x86 + vcan 虚拟 CAN 上跑通，再迁到 i.MX6ULL。**

**这份 sample 的真实行为（必须如实转述，不要夸大）：**

- **右电机**：RPDO1 COB-ID = `0x38A`，映射 `0x6041:00`(状态字,16bit) + `0x6064:00`(实际位置,32bit)。
- **左电机**：RPDO2 COB-ID = `0x38B`，同样映射 `0x6041` + `0x6064`。
- 通过 OD extension 回调 `H6064_write()` 把收到的位置 printf 出来：`posR = ...` / `posL = ...`。
- **诚实结论**：应用层被精简到只剩「接收并打印左右电机位置」。它是**真实可跑的双 OD + SYNC + RPDO 设备层骨架**，
  不是完整导航/运动控制程序。**读懂它 + 做出增量** 才是简历价值。

---

## 三、现在在干什么（当前任务）

**当前阶段：等电机到货（硬件空档期）—— 任务：用「不需要电机」的手段继续推进 + 为真机联调做准备。**

- 背景：用户**还没买到/收到电机**，真机此刻无法转，但这**不影响软件链路**（本工程是从站设备工程，
  除「电机真转 + 真实位置回传」外，整条链路用 **vcan 虚拟总线**即可跑通）。详见**第十二节**。
- 并行事项 A：**做"没电机也能学/能做"的清单落地**（vcan 全链、手敲报文、里程计、Qt HMI）。
- 并行事项 B：**硬件采购**（型号、渠道、闲鱼候选、必确认项）。详见**第十三节**。
- 等候事项 C：**真机 500K 打通**（森创出厂 250K/站址 1 与工程 500K/node 10·11 冲突），
  以及「总线无第二节点 ACK」导致 `ERROR-WARNING` 的问题——必须等电机到位才能实测。

> ⚠️ 上一轮的「写第 2 站分册」任务**已关闭**：`AGV_CANOPEN_LEARNING_MAP_S2_WALKTHROUGH_print.html` 已落地，
> 且 S3~S8 分册 + 面试 QA 也一并在目录中了（详见 4.1 与第五节）。
> 第七节保留为「第 2 站分册的原始提纲与取证数据」，作为**已归档参考**，不必再写一遍。

---

## 四、产出物体系与排版规范（分册必须照抄）

### 4.1 已有产出物（目录 `/home/zcy/Project/panasonic_canopen_agv/`）

| 文件 | 内容 |
|---|---|
| `AGV_CANOPEN_LEARNING_MAP_print.html` | **主手册**（911 行）：§0 十站总览 + 第 0 站（工程全景）+ 第 1 站（CAN 总线 & SocketCAN） |
| `AGV_CANOPEN_LEARNING_MAP_S1_WALKTHROUGH_print.html` | **第 1 站补充册**（304 行）：逐行慢读（链 A 建 socket / 链 B 收帧 / 发送重传） |
| `AGV_CANOPEN_LEARNING_MAP_S2_WALKTHROUGH_print.html` | **第 2 站补充册**：报文层（COB-ID 总账 + NMT/心跳/SYNC/EMCY） |
| `AGV_CANOPEN_LEARNING_MAP_S3_WALKTHROUGH_print.html` | **第 3 站**：对象字典 OD & SDO |
| `AGV_CANOPEN_LEARNING_MAP_S4_WALKTHROUGH_print.html` | **第 4 站**：PDO 映射 & CiA402 |
| `AGV_CANOPEN_LEARNING_MAP_S5_WALKTHROUGH_print.html` | **第 5 站**：启动链路走读（`CO_main_basic.c`） |
| `AGV_CANOPEN_LEARNING_MAP_S6A_RT_THREAD_print.html` | **第 6 站 A**：实时性改造 —— RT 线程 |
| `AGV_CANOPEN_LEARNING_MAP_S6B_RT_JITTER_print.html` | **第 6 站 B**：实时性改造 —— 抖动实测 |
| `AGV_CANOPEN_LEARNING_MAP_S7_ODOMETRY_print.html` | **第 7 站**：里程计（差速运动学 → 位姿 x,y,θ） |
| `AGV_CANOPEN_LEARNING_MAP_S8_QT_HMI_print.html` | **第 8 站**：Qt 上位机（显示位姿 / 发指令） |
| `AGV_CANOPEN_INTERVIEW_QA_print.html` | **面试 QA 册**：面向简历/面试的问答整理 |
| `MUSICPLAYER_CHAIN_QT_CPP_print.html` | **格式权威模板**（用户给的参考文件，不是本项目内容，只借排版，2629 行） |
| `HANDOFF.md` | 本文件 |

> 说明：S2~S8 分册与面试 QA **均已在目录落地**（旧版交接记的「S2 未开始 / S3~S9 未开始」已过期）。
> **第 9 站（交叉编译 & 部署）尚无独立分册**，其余十站已覆盖。

### 4.2 主手册的十站地图（阶段 A 摸清现状 / B 做增量 / C 上位机与上板）

```
【阶段 A：摸清现状 —— 先跑起来，再彻底看懂】           （不改代码，只读）
 第 0 站  工程全景        ← 主手册已写
 第 1 站  CAN 总线 & SocketCAN   ← 主手册已写 + S1 分册已写
 第 2 站  CANopen 报文层（NMT / 心跳 / SYNC / COB-ID）   ← 主手册已写（浅）；S2 分册待写 ★当前任务
 第 3 站  对象字典 OD & SDO
 第 4 站  PDO 映射 & CiA402      ◀── 全工程的心脏

【阶段 B：做出增量 —— 简历上的硬货】                    （开始改代码）
 第 5 站  启动链路走读（CO_main_basic.c）
 第 6 站  实时性改造（SCHED_FIFO + 抖动实测）
 第 7 站  里程计（差速运动学 → 位姿 x,y,θ）

【阶段 C：上位机 & 上板 —— 闭环成"产品"】
 第 8 站  Qt 上位机（显示位姿 / 发指令）
 第 9 站  交叉编译 & 部署到 imx6ull
```

### 4.3 分册 HTML 的排版规范（从 `..._S1_WALKTHROUGH_print.html` 照抄）

**组织范式（每个"站点"固定三段式）：**

1. 先给**完整代码块**：`<pre>` 首行写 `文件：xxx.c`，每行带原始行号前缀 `:NNN`。
2. 紧跟**四列逐行对照表** `<table class="cols-4c">`，列 = `行号 / 代码 / 用到的接口 / 变量与说明`。
   单元格内标识符用 `<code>` 包裹；接口列写 `接口名 ★ / ★★ / ★★★`。
3. 再配语义框：蓝色知识框 `.cpp`、灰色补充框 `.note`、红色坑框 `.warn`。
4. 章标题用 `<h2 class="pgbreak">` 强制分页（除首节）。

**★ 标记法**：`★★★` = 本项目核心必须会；`★★` = 重要；`★` = 知道即可。

**CSS（直接复制 S1 的 `<style>` 块）：**

- `@page { size:A4; margin:14mm 12mm 16mm 12mm; }`、`*{box-sizing:border-box}`、
  `html{-webkit-print-color-adjust:exact;print-color-adjust:exact}`
- `body` 10pt / line-height 1.55 / `max-width:190mm` / `padding:6mm 0`
- `h1` 17pt（下边框 2.5pt solid #111）；`h2` 13pt（左边框 4pt solid #111 + 灰底 #f0f0f0 + `page-break-after:avoid`）；
  `h3` 11pt；`h4` 10pt
- `code` 8.6pt 灰底 #f2f2f2；`pre` 7.6pt / 1.32 / 边框 0.6pt #bbb / 左边框 2.5pt solid #666 / `page-break-inside:avoid`
- `table` 8.8pt + `table-layout:fixed`；`caption` 顶部左对齐 9.5pt 粗体；`th` 底 #e8e8e8；
  `tr{page-break-inside:avoid}`；`thead{display:table-header-group}`
- **S1 里已定义的列宽类**：`.cols-3b`(19/25/56)、`.cols-4c`(9/27/26/38)。
  **S2 需要自己补列宽类**，建议：`.cols-3a`(18/34/48)、`.cols-4a`(18/16/16/50)、`.cols-4b`(11/25/22/42)、`.cols-4d`(22/20/32/26)
- 语义框：`.meta`（虚线框）、`.note`（左边框 3pt #333 底 #f7f7f7）、`.warn`（左边框 3pt #b33 底 #fdf5f5）、
  `.cpp`（左边框 3pt #2255a4 底 #f4f7fc，首行 `<strong>` 深蓝 = **简历话术**）、
  `.toolbar`（`@media print{display:none}`）、`.footnote`（上边框 0.6pt 虚线 8pt #444）、`.pgbreak`、`.star`
- 顶部固定 `<div class="toolbar"><button onclick="window.print()">打印 / 导出 PDF</button></div>`

---

## 五、进度总表

| 环节 | 状态 | 说明 |
|---|---|---|
| 选底座 | ✅ 完成 | 松下 CANopenLinux sample1 |
| 工程 clone | ✅ 完成 | `CANopenNode/` 是 tarball 手工落地（非真子模块），commit `1191979f...` |
| 编译（x86） | ✅ 完成 | 平铺编译 28 个 `.c`，链接出 `canopend`（521712 B）；`cocomm_cli`（35080 B） |
| vcan 虚拟总线 | ✅ 完成 | `can0` = vcan，状态 `<NOARP,UP,LOWER_UP> mtu 72` |
| 实跑 + 抓包 | ✅ 完成 | 抓到 `701 [1] 00` → `081 [8] 1081101400000000` → 每 100 ms 一帧 `080 [0]` |
| 主手册 §0 + 第 0 站 + 第 1 站 | ✅ 完成 | 911 行 |
| 第 1 站补充册（S1） | ✅ 完成 | 304 行，含过滤器语义争议的机制级纠错 |
| 第 2 站补充册（S2） | ✅ 完成 | 报文层（COB-ID / NMT / 心跳 / SYNC / EMCY）；原始提纲见第七节（归档） |
| 第 3~8 站分册 | ✅ 完成 | S3 OD&SDO / S4 PDO&CiA402 / S5 启动链路 / S6A+B 实时性 / S7 里程计 / S8 Qt |
| 面试 QA 册 | ✅ 完成 | `AGV_CANOPEN_INTERVIEW_QA_print.html` |
| **第 9 站（交叉编译 & 部署）分册** | ⬜ 未开始 | 十站中唯一没有独立分册的一站 |
| **硬件（电机）采购** | 🔶 进行中 | 型号/渠道/闲鱼候选见第十三节；**尚未到货** |
| **真机 500K 联调** | ⬜ 阻塞 | 等电机到位；问题见第十二节末、第十节 |

### 已解决的技术争议（结论已写入 S1，勿再翻案）

- 用户此前提供的 AI 回答称 `setsockopt(fd, SOL_CAN_RAW, CAN_RAW_FILTER, NULL, 0)` 是「清空过滤器、接收所有帧」。
  **该说法错误。** 本机 5 socket 实测 + Linux v5.15 `net/can/raw.c` 源码双向裁决：
  `optlen=0` → `count=0` → 空列表 → **一帧都不收（"闭麦"）**。S1 表述正确；该 AI 回答其余部分均正确。

---

## 六、已完成的技术工作（如实转述用）

### 6.1 编译方式（沙箱踩坑，必须沿用）

- 沙箱**禁止写子目录**（`CANopenNode/301/*.o`、`application/*.o` 被拦），但**工程根目录可写**。
- 解决办法：把 28 个源文件**平铺编译到根目录**，输出 `<basename>.o`。

```bash
CF="-Wall -g -ggdb -DCO_SINGLE_THREAD=1 -DCO_MULTIPLE_OD -DCO_USE_APPLICATION=1 \
    -DCO_CONFIG_HB_CONS=0 -DCO_CONFIG_STORAGE=0 -Wno-format -I. -ICANopenNode -Iapplication"
# SRCS = 28 个 .c，逐个：cc $CF -c "$s" -o "$(basename ${s%.c}).o"
cc -g -ggdb *.o -o canopend
```

28 个源文件清单：
`CO_driver.c CO_error.c CO_epoll_interface.c CO_storageLinux.c`
`CANopenNode/301/{CO_ODinterface,CO_NMT_Heartbeat,CO_HBconsumer,CO_Emergency,CO_SDOserver,CO_SDOclient,CO_TIME,CO_SYNC,CO_PDO,crc16-ccitt,CO_fifo}.c`
`CANopenNode/303/CO_LEDs.c`
`CANopenNode/304/{CO_GFC,CO_SRDO}.c`
`CANopenNode/305/{CO_LSSslave,CO_LSSmaster}.c`
`CANopenNode/309/CO_gateway_ascii.c`
`CANopenNode/storage/CO_storage.c` `CANopenNode/extra/CO_trace.c` `CANopenNode/CANopen.c`
`application/{OD,OD_2nd}.c` `CO_main_basic.c CO_application.c`

### 6.2 vcan 与实跑

```bash
sudo modprobe vcan
sudo ip link add dev can0 type vcan      # 已存在则跳过
sudo ip link set up can0
ip -details link show can0               # can0: <NOARP,UP,LOWER_UP> mtu 72 ... vcan

cd /home/zcy/Project/panasonic_canopen_agv
./canopend can0 -i 1                     # NodeID = 1
candump -td can0                         # 另开终端
```

### 6.3 抓包证据链（本工程真实运行结果）

```
701   [1] 00                      ← 开机自报家门（boot-up；data[0]=0x00 = INITIALIZING，发完才切 OPERATIONAL）
081   [8] 10 81 10 14 00 00 00 00 ← EMCY：errorCode=0x8110(CO_EMC_CAN_OVERRUN)、errorReg=0x10(COMMUNICATION)、errorBit=0x14(CO_EM_CAN_TX_OVERFLOW)
080   [0]                         ← 每 100 ms 一帧（SYNC 生产者）
```

### 6.4 沙箱假故障（必须告诉用户：真机不会有这条 EMCY）

- 现象：`send()` 返回 16（成功）却走进 `CO_driver.c` 的 else 兜底分支 → 置 `CO_CAN_ERRTX_OVERFLOW` → 报 EMCY `0x8110`。
- 根因：**TRAE 沙箱的 `LD_PRELOAD=sbox.so` 把 `errno` 改写为 95（EOPNOTSUPP）**，而 `send()` 实际成功。
- 结论：这是**沙箱环境缺陷**，不是工程 bug，真机 / 用户自己终端跑不会出现。

---

## 七、第 2 站分册：可直接照写的提纲 + 已核实的全部数据

> 以下数据**已逐条核对过源码行号与原文**，接手者直接组织成 HTML 即可，不必重新读源码。
> 写作硬要求：**逐行交代「代码作用」+「关键变量的作用」**，凝练，表格为主。

### §1 一页地图：开机后总线上真实出现的 4 种帧

用 `<pre>` 画「本机 candump 实测的 4 帧 + 对照：能力存在但本工程未触发的帧」。
4 帧即 §6.3 的 `701 / 081 / 080`（第 4 帧类型是 LSS `7E4/7E5`，视有无主站而定）。

### §2 COB-ID 总账

- **2.1 预定义 ID 表** —— 出处 [CO_driver.h:479-499](file:///home/zcy/Project/panasonic_canopen_agv/CANopenNode/301/CO_driver.h#L479-L499)：
  `NMT_SERVICE=0x000`、`GFC=0x001`、`SYNC=0x080`、`EMERGENCY=0x080(+nodeID)`、`TIME=0x100`、`SRDO_1=0x0FF`、
  `TPDO_1=0x180`、`RPDO_1=0x200`、`TPDO_2=0x280`、`RPDO_2=0x300`、`TPDO_3=0x380`、`RPDO_3=0x400`、
  `TPDO_4=0x480`、`RPDO_4=0x500`、`SDO_SRV=0x580`、`SDO_CLI=0x600`、`HEARTBEAT=0x700`、`LSS_SLV=0x7E4`、`LSS_MST=0x7E5`。
  附 [CO_driver.h:508-515](file:///home/zcy/Project/panasonic_canopen_agv/CANopenNode/301/CO_driver.h#L508-L515) 的 `CO_IS_RESTRICTED_CAN_ID`（受限 ID 不得用于 SYNC/TIME/EMCY/PDO/SDO）。
- **2.2 本工程 rx/tx 槽位账本** —— 出处 [CANopen.c:257-286](file:///home/zcy/Project/panasonic_canopen_agv/CANopenNode/CANopen.c#L257-L286)
  的索引宏链（注释说明「按 CAN ID 优先级排序」）。**逐宏展开结论（已修正过一次，务必用这版）**：
  - **rxSize = 20**：`NMT 1 + SYNC 1 + EM_CONS 1 + TIME 1 + GFC 0 + SRDO 0 + RPDO 4 + SDO_SRV 1 + SDO_CLI 1 + HB_CONS 8 + LSS_SLV 1 + LSS_MST 1`
    （注意 **SDO_CLI = 1**：`CO_CONFIG_SDO_CLI_ENABLE` 已在 `CO_driver_target.h:105-113` 开启，`OD_CNT_SDO_CLI=1` 见 `application/OD.h:45`。
    旧交接里的 `rxSize=19` 是**错的**。）
    索引落点：`CO_RX_IDX_NMT_SLV=0`、`SYNC=1`、`EM_CONS=2`、`TIME=3`、`GFC=4`、`SRDO=4`、`RPDO=4`、`SDO_SRV=8`、
    `SDO_CLI=9`、`HB_CONS=10`、`LSS_SLV=18`、`LSS_MST=19`、`CO_CNT_ALL_RX_MSGS=20`。
  - **txSize = 13**：`NMT_MST 1 + SYNC 1 + EM_PROD 1 + TIME 1 + TPDO 4 + SDO_SRV 1 + SDO_CLI 1 + HB_PROD 1 + LSS_SLV 1 + LSS_MST 1`
    （`CO_TX_IDX_NMT_MST=0`、`SYNC=1`、`EM_PROD=2`、`TIME=3`、`TPDO=4`、`SDO_SRV=8`、`SDO_CLI=9`、`HB_PROD=10`、`LSS_SLV=11`、`LSS_MST=12`）
  - **HB_CONS = 8** 来自 `OD_CNT_ARR_1016 = 8`（`application/OD.h:56`），用于心跳消费者 8 个槽。
  - 文档里须标注这是**宏推算值**，不是实测值。
- **2.3 注册顺序** —— 出处 [CANopen.c:951-1087](file:///home/zcy/Project/panasonic_canopen_agv/CANopenNode/CANopen.c#L951-L1087)：
  `CO_EM_init` → `CO_NMT_init` → `CO_HBconsumer_init` → `CO_SDOserver_init`(循环) → `CO_SDOclient_init`(循环)
  → `CO_TIME_init` → `CO_SYNC_init` → `CO_GFC_init`。逐段说明"每个模块从哪里拿到自己的 CAN 槽位与 COB-ID"。

### §3 NMT（网络管理）

- **3.1 收** —— [CO_NMT_Heartbeat.c:36-54](file:///home/zcy/Project/panasonic_canopen_agv/CANopenNode/301/CO_NMT_Heartbeat.c#L36-L54)：
  `DLC==2 && (nodeId==0 || nodeId==NMT->nodeId)` 才收；只做一件事 `NMT->internalCommand = command;`
- **3.2 处理** —— [CO_NMT_Heartbeat.c:219-325](file:///home/zcy/Project/panasonic_canopen_agv/CANopenNode/301/CO_NMT_Heartbeat.c#L219-L325)：
  `internalCommand` 的 switch（L259-280）、错误降级判定（L282-298）、状态变化回调（L300-307）、返回 `resetCommand`。
- **枚举表**（出处 [CO_NMT_Heartbeat.h:77-158](file:///home/zcy/Project/panasonic_canopen_agv/CANopenNode/301/CO_NMT_Heartbeat.h#L77-L158)）：
  - 状态：`UNKNOWN=-1`、`INITIALIZING=0`、`PRE_OPERATIONAL=127`、`OPERATIONAL=5`、`STOPPED=4`
  - 命令：`NO_COMMAND=0`、`ENTER_OPERATIONAL=1`、`ENTER_STOPPED=2`、`ENTER_PRE_OPERATIONAL=128`、`RESET_NODE=129`、`RESET_COMMUNICATION=130`
  - reset 返回码：`NOT=0`、`COMM=1`、`APP=2`、`QUIT=3`
  - control 位域：`ERR_REG_MASK=0x00FF`、`STARTUP_TO_OPERATIONAL=0x0100`、`ERR_ON_BUSOFF_HB=0x1000`、
    `ERR_ON_ERR_REG=0x2000`、`ERR_TO_STOPPED=0x4000`、`ERR_FREE_TO_OPERATIONAL=0x8000`
- **`.warn` 坑**：`CO_NMT_receive()`（中断/回调上下文）**只记录命令，不立即执行**，真正切换在 `CO_NMT_process()`。
- **`.note` 本工程 NMTcontrol = 0x2111** —— 出处 [CO_main_basic.c:66-73](file:///home/zcy/Project/panasonic_canopen_agv/CO_main_basic.c#L66-L73)：
  `STARTUP_TO_OPERATIONAL | ERR_ON_ERR_REG | CO_ERR_REG_GENERIC_ERR | CO_ERR_REG_COMMUNICATION`
  = `0x0100 | 0x2000 | 0x01 | 0x10` = **0x2111**。
  含义：① 开机自启动进 OPERATIONAL；② 但 error register 命中 mask（bit0/bit4）就退回 PRE_OPERATIONAL
  （因 `ERR_TO_STOPPED` 未设）。**这正好解释了 §6.4 沙箱假故障下节点被拉回 pre-op 的现象。**
- **3.3 发（仅主站能力）** —— [CO_NMT_Heartbeat.c:330-349](file:///home/zcy/Project/panasonic_canopen_agv/CANopenNode/301/CO_NMT_Heartbeat.c#L330-L349)：
  `NMT_TXbuff->data[0]=command; data[1]=nodeID;`（DLC=2）。`CO_CONFIG_NMT_MASTER` 已开（`CO_driver_target.h:67-72`）。

### §4 心跳（Heartbeat / boot-up）

- **4.1 初始化** —— [CO_NMT_Heartbeat.c:114-186](file:///home/zcy/Project/panasonic_canopen_agv/CANopenNode/301/CO_NMT_Heartbeat.c#L114-L186)：
  `operatingState = operatingStatePrev = CO_NMT_INITIALIZING`；`HBproducerTimer = firstHBTime_ms*1000`；
  从 OD `0x1017` 读 `HBprodTime_ms` → `HBproducerTime_us`；
  `CO_CANrxBufferInit(..., CANidRxNMT=0x000, mask=0x7FF, ..., CO_NMT_receive)`；
  `HB_TXbuff = CO_CANtxBufferInit(..., CANidTxHB=0x700+nodeId, rtr=0, **DLC=1**, 0)`。
- **4.2 OD 写回调** —— [CO_NMT_Heartbeat.c:62-79](file:///home/zcy/Project/panasonic_canopen_agv/CANopenNode/301/CO_NMT_Heartbeat.c#L62-L79)：
  写 `0x1017` 时同步更新 `HBproducerTime_us`、把 `HBproducerTimer=0`（**立即发一次心跳**），最后落 OD。
- **4.3 发送条件** —— [CO_NMT_Heartbeat.c:229-255](file:///home/zcy/Project/panasonic_canopen_agv/CANopenNode/301/CO_NMT_Heartbeat.c#L229-L255)：
  `if (NNTinit || (HBproducerTime_us != 0 && (HBproducerTimer==0 || state != statePrev)))` → `HB_TXbuff->data[0]=state; CO_CANsend(...)`。
- **`.note` 本工程 `0x1017 = 0`**（`application/OD.c:33`）⇒ `HBproducerTime_us = 0` ⇒ **周期性心跳关闭，只在启动发一次 boot-up**，与实测吻合。
- **`.note` boot-up 帧内容 = 0x00 而非 0x7F**：发送时 `NMTstateCpy` 仍为 `INITIALIZING(0)`，发完才在 L242-248 切成 OPERATIONAL/PRE_OPERATIONAL。
- **`.note` `FIRST_HB_TIME = 500` ms**（`CO_main_basic.c:74-76`），只影响首次定时器初值；因 `HBproducerTime_us=0`，实际只发一次。

### §5 SYNC

- **5.1 初始化** —— [CO_SYNC.c:272-336](file:///home/zcy/Project/panasonic_canopen_agv/CANopenNode/301/CO_SYNC.c#L272-L336)：
  读 `0x1019`（1→2、>240→240）；`isProducer = (cobIdSync & 0x40000000) != 0`；`CAN_ID = cobIdSync & 0x7FF`；
  `CO_CANrxBufferInit(..., cobIdSync & 0x7FF, 0x7FF, ...)`；`CANtxBuff` 的 **DLC = (0x1019 != 0) ? 1 : 0**。
- **5.2 收** —— [CO_SYNC.c:37-74](file:///home/zcy/Project/panasonic_canopen_agv/CANopenNode/301/CO_SYNC.c#L37-L74)：
  `counterOverflowValue==0` 时要求 `DLC==0`，否则 `receiveError = DLC | 0x40`；不为 0 时要求 `DLC==1` 并取 `data[0]` 到 `counter`，
  否则 `receiveError = DLC | 0x80`。收到则 **`CANrxToggle` 翻转（PDO 双缓冲切换）** + `CO_FLAG_SET(CANrxNew)`。
- **5.3 处理** —— [CO_SYNC.c:356-457](file:///home/zcy/Project/panasonic_canopen_agv/CANopenNode/301/CO_SYNC.c#L356-L457)：
  定时器累加 → 收到则清零并置 `CO_SYNC_RX_TX` → 生产者按 `0x1006` 周期发 → 超时判定 `periodTimeout = 0x1006*1.5`（L403）
  → 报 `CO_EM_SYNC_TIME_OUT` → 同步窗口 `0x1007` 判定 `CO_SYNC_PASSED_WINDOW` → `receiveError` 报
  `CO_EM_SYNC_LENGTH`（错误码 `CO_EMC_SYNC_DATA_LENGTH`）。
- **`.note` OD 值**（`CO_application.c:76,82` + `application/OD.c:25-27,41`）：
  - `0x1006 = 100000` µs = **100 ms**（应用层运行期写入）
  - `0x1005 = 0x40000080`（应用层运行期写入；**bit30=1 ⇒ 本节点是 SYNC 生产者**，CAN_ID = 0x080）
  - `0x1019 = 0` ⇒ SYNC 帧 **DLC = 0**（实测 `080 [0]` 的原因）
  - `0x1007 = 0`（无同步窗口）
- **`.note` 为什么 `080` 每 100 ms 一帧**：`isProducer=1` 且 `0x1006=100000` ⇒ `CO_SYNC_process()` 到点就 `CO_SYNCsend()`。

### §6 EMCY（应急报文）

- **6.1 初始化** —— [CO_Emergency.c:412-466](file:///home/zcy/Project/panasonic_canopen_agv/CANopenNode/301/CO_Emergency.c#L412-L466)：
  读 `0x1014`（`COB_IDEmergency32`），校验 `(x & 0x7FFFF800) == 0`；
  `producerEnabled = (x & 0x80000000)==0 && producerCanId != 0`；
  **`if (producerCanId == CO_CAN_ID_EMERGENCY=0x80) producerCanId += nodeId;`**；
  `CANtxBuff = CO_CANtxBufferInit(..., producerCanId, 0, **DLC=8**, 0)`。
  本工程 `0x1014 = 0x00000080`（`application/OD.c:29`）⇒ 实际 COB-ID = **0x080 + nodeId = 0x081**（nodeId=1）。
- **6.2 驱动错误 → EMCY 映射** —— [CO_Emergency.c:561-595](file:///home/zcy/Project/panasonic_canopen_agv/CANopenNode/301/CO_Emergency.c#L561-L595)：
  只有 `CANerrSt != CANerrorStatusOld` 才处理。关键一行：
  `if (CANerrStChanged & CO_CAN_ERRTX_OVERFLOW) CO_error(em, ..., CO_EM_CAN_TX_OVERFLOW, CO_EMC_CAN_OVERRUN, 0);`
- **6.3 组帧 + 发送** —— [CO_Emergency.c:746-775](file:///home/zcy/Project/panasonic_canopen_agv/CANopenNode/301/CO_Emergency.c#L746-L775)
  与 [CO_Emergency.c:596-650](file:///home/zcy/Project/panasonic_canopen_agv/CANopenNode/301/CO_Emergency.c#L596-L650)：
  - `errMsg = (uint32_t)errorBit << 24 | CO_SWAP_16(errorCode);`（L748）
  - **`CO_SWAP_16(x)` 在 x86 小端下是恒等**（`CO_driver.h:139-145`：`#define CO_LITTLE_ENDIAN` / `#define CO_SWAP_16(x) x`）
  - error register 在 post-process 里补进 byte2：`em->fifo[..].msg |= (uint32_t)errorRegister << 16;`（L644）
  - `memcpy(CANtxBuff->data, &msg, 8)` 只拷 4 字节，**byte4~7 保持 0**
  - error register 计算：`CO_CONFIG_ERR_CONDITION_GENERIC`→bit0、`..._COMMUNICATION`→bit4（L596-620）
- **6.4 字节级破解实测帧 `081 [8] 10 81 10 14 00 00 00 00`**（这是本册最有价值的一段，务必写全）：

  | data[i] | 值 | 含义 | 出处 |
  |---|---|---|---|
  | `[0]`,`[1]` | `10 81` | errorCode = `0x8110` = `CO_EMC_CAN_OVERRUN`（小端） | `CO_Emergency.h:178` |
  | `[2]` | `10` | error register = `CO_ERR_REG_COMMUNICATION`(bit4) | `CO_Emergency.h:122` |
  | `[3]` | `14` | errorBit = `CO_EM_CAN_TX_OVERFLOW` | `CO_Emergency.h` 内部错误位表 |
  | `[4..7]` | `00×4` | infoCode 为 0 / 未拷贝 | — |

  完整因果链：沙箱 `LD_PRELOAD` 把 `errno` 改成 95 → [CO_driver.c:753-760](file:///home/zcy/Project/panasonic_canopen_agv/CO_driver.c#L753-L760) 兜底分支
  `CANerrorStatus |= CO_CAN_ERRTX_OVERFLOW` → [CO_Emergency.c:580-582](file:///home/zcy/Project/panasonic_canopen_agv/CANopenNode/301/CO_Emergency.c#L580-L582)
  `CO_error(CO_EM_CAN_TX_OVERFLOW, CO_EMC_CAN_OVERRUN)` → 组帧发出。
- **`.warn` 沙箱假故障**：这是**环境缺陷**，真机 / 用户终端不会出现（详见 §6.4）。
- **`.note` 为什么 bit4(COMMUNICATION) 被置起** —— [CO_Emergency.h:42-52](file:///home/zcy/Project/panasonic_canopen_agv/CANopenNode/301/CO_Emergency.h#L42-L52)：
  `CO_CONFIG_ERR_CONDITION_COMMUNICATION = (errorStatusBits[2] || errorStatusBits[3])`，
  `errorStatusBits[2]` 覆盖错误位 16..23，包含 `0x14`(TX overflow) ⇒ 通信错误位被假故障点亮。

### §7 关键变量总表（用户明确要求"关键变量的作用"）

- **`CO_NMT_t`** —— [CO_NMT_Heartbeat.h:164-204](file:///home/zcy/Project/panasonic_canopen_agv/CANopenNode/301/CO_NMT_Heartbeat.h#L164-L204)：
  `operatingState` / `operatingStatePrev` / `internalCommand` / `nodeId` / `NMTcontrol` /
  `HBproducerTime_us` / `HBproducerTimer` / `OD_1017_extension` / `em` / [`NMT_CANdevTx`,`NMT_TXbuff`] / `HB_CANdevTx` / `HB_TXbuff` /
  [`pFunctSignalPre`,`functSignalObjectPre`] / `pFunctNMT`
- **`CO_SYNC_t`** —— [CO_SYNC.h:81-143](file:///home/zcy/Project/panasonic_canopen_agv/CANopenNode/301/CO_SYNC.h#L81-L143)：
  `em` / `CANrxNew` / `receiveError` / `CANrxToggle` / `timeoutError` / `counterOverflowValue` / `counter` /
  `syncIsOutsideWindow` / `timer` / `OD_1006_period`(*) / `OD_1007_window`(*) / `isProducer` / `CANtxBuff` /
  [`CANdevRx`,`CANdevRxIdx`,`OD_1005_extension`,`CAN_ID`,`CANdevTx`,`CANdevTxIdx`,`OD_1019_extension`]
- **`CO_CANrx_t` / `CO_CANtx_t`** —— [CO_driver.h:263-272](file:///home/zcy/Project/panasonic_canopen_agv/CANopenNode/301/CO_driver.h#L263-L272) 与
  [CO_driver.h:305-313](file:///home/zcy/Project/panasonic_canopen_agv/CANopenNode/301/CO_driver.h#L305-L313)：
  `ident`(bit0..10 ID + bit11 RTR) / `mask` / `object` / `pCANrx_callback`；
  `ident` / `DLC` / `data[8]` / `bufferFull` / `syncFlag`

### §8 自测题 + 第 3 站预告 + `.footnote`

自测例：① 本工程 rxSize/txSize 各多少、为什么？② `0x701` 里 data[0] 为什么是 0x00？③ `080` 为什么每 100 ms 一帧？
④ 破解 `081` 那一帧的 4 个字节。⑤ 本节点为什么先 OPERATIONAL 又回 PRE_OPERATIONAL？
第 3 站预告：对象字典 OD & SDO。

---

## 八、关键技术速查表

### CANopen 默认 COB-ID（预定义连接集）

| 功能 | COB-ID |
|---|---|
| NMT | 0x000 |
| SYNC | 0x080 |
| EMCY | 0x080 + nodeId |
| TPDO1 | 0x180 + nodeId |
| RPDO1 | 0x200 + nodeId |
| SDO 响应 | 0x580 + nodeId |
| SDO 请求 | 0x600 + nodeId |
| 心跳 / NodeGuarding | 0x700 + nodeId |

### NMT 命令字 / 状态

- 命令：start=1、stop=2、preop=0x80、reset_node=0x81、reset_comm=0x82
- 状态：boot-up=0、stopped=4、operational=5、pre-operational=127

### CiA 402（本工程核心）

- 对象：控制字 `0x6040`、状态字 `0x6041`、模式 `0x6060`/`0x6061`、目标位置 `0x607A`、实际位置 `0x6064`、
  目标速度 `0x60FF`、实际速度 `0x606C`、轮廓速度 `0x6081`、轮廓加速度 `0x6083`、目标转矩 `0x6071`。
- 状态机命令：Shutdown=0x06、Switch On=0x07、Enable Operation=0x0F、Quick Stop=0x02、Fault Reset=0x80。
- 运行模式：NoMode=0、PP=1、VL=3、PT=4、Homing=6、IP=7、CSP=8、CSV=9、CST=10。
- 本工程 PDO 映射：`0x6041:00`(16bit) + `0x6064:00`(32bit) → 映射值 `0x60410010` / `0x60640020`
  （高 16 位=索引，次 8 位=子索引，最低 8 位=位长）。

### 本工程关键标识

- 从站节点 = CANopen 设备；RPDO COB-ID：**`0x38A`(右)** / **`0x38B`(左)**（`CO_application.c:91,121`）。
- 两套对象字典：`OD`(右) + `OD_2nd`(左)，靠 `-DCO_MULTIPLE_OD` 启用。
- 单线程模式 `-DCO_SINGLE_THREAD=1`：RT 线程不启用，`app_programRt()` 不会被调用。

---

## 九、环境实测信息（避免重复踩坑）

- OS：Ubuntu 20.04.6 LTS (Focal)、x86_64；内核 5.15.0-139-generic
- gcc/g++ 9.4.0、CMake 3.16.3、GNU Make 4.2.1
- 交叉工具链：`/usr/local/arm/gcc-linaro-4.9.4-2017.01-x86_64_arm-linux-gnueabihf/bin/`
  （gcc 4.9.4 → 最高 C++14；本工程纯 C，无压力）
- 未安装：can-utils（**已确认需装才能 candump**）、libmodbus、lely
- `/opt/ros` 只有 **noetic（ROS 1）**，**没有 ROS 2**
- `vcan.ko` / `vxcan.ko` 内核模块存在；`can0` 已建成 vcan 且 UP
- 网卡：`ens37`、`ens38`、`lo`（+ `can0` vcan）
- 网络：`git clone` 直连 GitHub 卡死；**`codeload.github.com` 直连可用**；
  `ghproxy.net` 可代理 git clone，但转发 codeload 会 403。
- **sudo 需要密码**，AI 无法非交互执行 `modprobe` / `ip link` → 需用户手动执行。
- **TRAE 沙箱**：`LD_PRELOAD=sbox.so` 会改写 `errno`（见 §6.4）；且**禁止写子目录**（见 §6.1）。

---

## 十、已知遗留问题（待清理 / 待修）

1. `CANopenNode/` 是 tarball 解压的，**不是真正的 git 子模块**（沙箱禁止 `git submodule` 写 `.git/modules`）。
2. 工程根目录平铺了 **29 个 `.o`** + `cocomm.o`，可手动清理：`rm -f *.o`（用户终端执行没问题）。
3. `canopend` 与 `cocomm_cli` 是**手工链接**产物，没走 `make install`；若重跑 `make`，注意其
   `all: clean $(LINK_TARGET)` 会先 clean，且会因沙箱禁写子目录而失败 → **继续沿用"平铺编译 + 手动链接"**。
4. （代码层，未修）多线程 `CO_2nd` 路径丢失：`CO_main_basic.c:561` 两个 CO 共用 `epRT.epoll_fd`，
   但 `rt_thread`（:891）只调 `CO_epoll_processRT(&epRT, CO, true)`。
5. （代码层，未修）`CO_2nd` 完整 `CO_process` 未执行：`CO_main_basic.c:806` 被注释。
   —— 第 5 站读启动链路时会遇到，届时要讲清楚（这也是可以做增量的点）。
6. 目录下残留 `CANopenNode-1191979f.../`（重复目录），沙箱拒绝删除，可忽略或用户手动 `rm -rf`。
7. （待验证）HMI 已改默认板卡 IP 为 `169.254.86.72`（`hmi/mainwindow.cpp:144`），但**尚未重新编译、
   也未在 PC 上运行验证**；无电机时可先连 `127.0.0.1` 连 vcan 上的网关测试。
8. （待用户向卖家确认）森创 9 芯插头 **1/2 脚是否为电源**，并索要**配套对接插头**。

---

## 十二、等电机期间：不用电机就能学/能做的事（2026-10-05 已答复用户）

**核心判断：本工程是 CANopen 从站（设备）工程，除「电机真转 + 真实位置回传」外，
整条软件链路用 vcan 虚拟总线即可跑通、可学、可写进简历。**

### 12.1 现在就能做（按推荐顺序）

1. **vcan 上把整条链跑通**（最核心）。`canopend` 的 `-c` 网管接口支持 `tcp-<port>` / `local-<file>`，
   **不依赖任何物理设备**，出处 [CO_main_basic.c:251-256](file:///home/zcy/Project/panasonic_canopen_agv/CO_main_basic.c#L251-L256)。

   ```bash
   sudo modprobe vcan
   sudo ip link add dev can0 type vcan
   sudo ip link set up can0
   cd /home/zcy/Project/panasonic_canopen_agv
   ./canopend can0 -i 1 -c "tcp-60000"     # vcan 无需 ACK
   ```

   vcan 下所有 PDO/OD 会真实装载：右电机 RPDO1 COB-ID `0x38A` + 映射 `0x60410010`/`0x60640020`
   见 [CO_application.c:123-138](file:///home/zcy/Project/panasonic_canopen_agv/CO_application.c#L123-L138)；
   左电机在第二套 OD 同位置，见 [CO_application.c:153-168](file:///home/zcy/Project/panasonic_canopen_agv/CO_application.c#L153-L168)。
2. **手敲报文练协议**（第 2 / 3 站）——报文层 + OD&SDO：

   ```bash
   candump -td can0
   cansend can0 000#0101              # NMT 进 OPERATIONAL
   cansend can0 000#8001              # 回 pre-operational
   cansend can0 601#4000180100000000  # SDO 读 0x1018，观察 581# 响应
   ```

   看 `080#` 每 100 ms 的 SYNC（因 `0x1017=0` 心跳关、本节点是 SYNC 生产者，见
   [OD.c:33](file:///home/zcy/Project/panasonic_canopen_agv/application/OD.c#L33) 与
   [CO_main_basic.c:66-73](file:///home/zcy/Project/panasonic_canopen_agv/CO_main_basic.c#L66-L73) 的 NMTcontrol=`0x2111`）。
3. **里程计（第 7 站）纯数学，零硬件**：自定义对象 `0x6FFF`(float32×3) 就是给它留的，见
   [OD.c:281-283](file:///home/zcy/Project/panasonic_canopen_agv/application/OD.c#L281-L283)。
4. **Qt HMI（第 8 站）提前做**：PC 上编译 `hmi/`，把 IP 改成 `127.0.0.1` 连 vcan 上的网关，
   验证「UI → 网关 → CAN」全链（默认板卡 IP 见 [mainwindow.cpp:144](file:///home/zcy/Project/panasonic_canopen_agv/hmi/mainwindow.cpp#L144)）。
5. **理论分册**：`S2~S8` 已存在目录，可直接精读。

### 12.2 必须等电机（真机）才能做

| 能力 | 原因 |
|---|---|
| 真实位置回传 | `0x6064` / `0x6041` 需电机侧实际值，vcan 只能回填假值 |
| 电机实际转动 | 使能 / 电流环要真机（本工程不研究算法，但要看得到动） |
| 真机总线 ACK | 500K 波特率 + 第二节点应答；**vcan 没有 ACK**，所以现在遇不到 `ERROR-WARNING` 刷屏 |

---

## 十三、硬件采购调研（2026-10-05）

**目标硬件**：森创（syn-tron / 北京和利时）**ISS56C250DR12**，CANopen 闭环步进一体机，**2 N·m**。

### 13.1 型号解码 & 出厂参数（真机联调关键）

- `ISS` = 一体集成式步进伺服；机座号(42/56/57/60/86)；`C` = 通讯格式(C=CANopen, R=RS485)；
  `2` = 相数；`50` = 齿数；机身长度字母；后缀 `R00/R11/R12`。
- **出厂默认波特率 250K、站址 1**；改波特率 = SDO `2000h` 子索引 4，改站址 = `2000h` 子索引 3（1-127），
  **改动需重新上电**。← 这与本工程 **500K / node 10·11** 冲突，是**真机联调第一坑**。

### 13.2 结论：线下几乎没有 CANopen 一体机现货

用户默认收货地 = 杭州钱塘 / 下沙。当地机电市场（金茂五金机电、长城机电、浙江科奥、
南方机电、西盟自动化、摩森机电等）以**配件**为主，**CANopen 一体机基本要订货加价**。
→ 结论：**优先线上（闲鱼同城自提 / 淘宝按发货地=浙江）**，线下市场只适合买配件。

### 13.3 闲鱼候选（按性价比）

| 型号 | 规格 | 价格 | 所在地 | 备注 |
|---|---|---|---|---|
| 森创 ISS56C250DR12 | CANopen 2 N·m，全新未用，带插头 | **¥372** | 北京 | **推荐（稳妥，同款）** id=1058119965225 |
| 森创 ISS56C250CR11 | CANopen 1.6 N·m | **¥80** | 河北 | **推荐（便宜试水）** id=1064824113999 |
| 杰美康 57 闭环 CAN | 3 N·m，17 位编码器 | ¥400 | 湖北 | 支持自提 id=1037472558205 |
| NiMotion BLM4205A | CANopen | ¥168 | 广东 | id=1052095906807 |
| 森创 ISS56C250DR12 | 标价面议 | ¥18.80 | 河北 | id=1027689814123 |
| 立迈胜 STM2832B | CANopen | ¥100 | 江苏 | 28 机座偏小 |
| 鸣志 17C-3CG-M-F02 | CANopen/CiA402 | ¥215 | 浙江 | 17 机座偏小 |
| 森创 ISS86C250DR00 | CANopen 8 N·m | ¥118 | 河北 | 86 机座太大 |

**推荐**：稳妥选 **¥372 全新同款 ISS56C250DR12**；便宜试水选 **¥80 ISS56C250CR11**。

### 13.4 下单前必确认

- 必须是 **CANopen**（不是只 RS485 / Modbus）；
- 机座 **57 / 56**，扭矩 **≥ 2 N·m**；
- 向卖家确认 **9 芯插头 1/2 脚是否为电源**，并**索要配套对接插头**。

---

## 十四、给下一个对话的一句话摘要

> 用户要在**别人做好的现成工程**上做嵌入式 CANopen/AGV 项目（不自研、可买便宜硬件、**不碰电机算法**、
> 要能写进简历、**分册推进、输出凝练**）。
> **新增硬要求（2026-10-05）**：讲代码**以函数为单位**（干什么/被谁调/调了谁）；**必须标 `文件:行号` +
> 必须配流程图**；变量**在哪出现就在那说明**；讲清**整个工程的运行链路**；**不用 `AskUserQuestion`**；**中文回复**。
> 底座 = **松下 CANopenLinux sample1（実験用 AGV）**，代码在 `/home/zcy/Project/panasonic_canopen_agv/`，
> 已用"平铺编译"编出 `canopend`（521712 B），已在 vcan `can0` 上**实跑并抓到 `701/081/080` 三类帧**。
> 文档 = **主手册 911 行 + S1~S8 分册 + 面试 QA**（十站中仅第 9 站交叉编译部署尚无独立分册）。
> **当前阶段：等电机到货**——用**不用电机**的手段继续推进（vcan 全链 / 手敲报文 / 里程计 / Qt HMI，
> 详见第十二节）；同时**硬件采购进行中**（森创 ISS56C250DR12，闲鱼候选与下单必确认项见第十三节）。
> 电机到位后再做**真机 500K 联调**（出厂 250K/站址 1 需改，且有"总线无第二节点 ACK"问题）。
> 必须如实告知：这份 sample 应用层被精简为"收并打印左右电机位置"，是真实可跑的设备层骨架，
> 完整导航/运动控制需作为增量补齐（这正是简历价值所在）。
