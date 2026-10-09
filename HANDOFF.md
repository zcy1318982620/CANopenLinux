# 项目交接说明（换对话 / 换任务前必读）

> 更新时间：**2026-10-07**
> 目的：把「用户需求与硬约束 / 已确定的路线 / 正在干什么 / 进度到哪 / 下一步怎么写」完整交接。
> 接手的人（或 AI）：**先完整读完本文件，再动手**。
>
> **本次（2026-10-07）更新要点（重要：纠正旧记录的滞后，请以此为准）**
>
> 1. **工程已跑在旧交接文档前面**。旧文档记「等电机空档」时，代码侧已落地一批增量：
>    - **异步统一日志模块**已实现并接入 `log_printf`（`agv_log.c/h` + `agv_queue.c/h` + `agv_pool.c/h`，见 6.5）。
>    - **CMake 构建 + ARM 交叉工具链**已就绪（`CMakeLists.txt` / `toolchain-arm.cmake` / `deploy/canopend_arm`，见 6.8）。
>    - **仿真从站 `tools/sim_motors.py`** 可零硬件回放 `0x38A`/`0x38B`（见 6.7）。
>    - **Qt HMI** 可经 CiA-309 网关轮询 `0x6FFF` 画轨迹，并可 **SDO 写 `0x7000` cmd_vel 下发运动指令**（上下行打通，见 6.6）。
> 2. **第 9 站分册已落地**：`docs/AGV_CANOPEN_LEARNING_MAP_S9_CROSS_COMPILE_DEPLOY_print.html`。
>    旧记录「第 9 站未开始」**已过期**；**新增 S11 逆运动学分册后，覆盖已到第 11 站**。
> 3. **`docs/` 已按类型扩充**：新增并发（队列/线程池）、日志与错误处理、嵌入式应用自检、CANopen 心智模型、
>    工具链构建/调试、异步日志实现等专题分册（见 4.1）。
> 4. **遗留问题已修**：旧 #4「RT 线程只处理 `CO`、丢了 `CO_2nd`」**已修**——`rt_thread` 现同时处理两者（见 6.10 与 第十节）。
> 5. **下行控制链路已补齐（纠正旧记录「完全缺失」）**：应用层已落地 **CiA402 使能时序（`0x6040`）+ `CO_NMT_sendCommand` 主站 Start +
>    TPDO `0x50A/0x50B` 下发 `0x6040`+`0x60FF`**，并新增 **逆运动学**（`0x7000` cmd_vel → `0x60FF`，含"未使能不动/失联停车"双安全门限）。
>    分册新增 **S11**。**vcan 闭环实跑验证已于 2026-10-07 PASS；P1 心跳容错亦已于 2026-10-07 落地并 vcan 验证 PASS（超时降级 + 恢复重连）**，详见第十五节「下一步增量路线」。
>    （旧记录称"下行控制链路完全缺失 / 无 `0x6040`/`CO_NMT_sendCommand`"**已过期**。）
>
> **本次（2026-10-05）更新要点：**
> 1. **分册进度大幅推进**：S2~S8 分册 + 面试 QA 均**已落地**（旧版记的「S2 未开始」已过期，见 4.1 / 第五节）。
> 2. **新增用户硬要求**（见第一节 13~18 条）：**以函数为单位讲代码 / 必须标 `文件:行号` 且必须有流程图 /
>    变量"在哪出现就在那说明" / 不再使用 `AskUserQuestion`**。
> 3. **进入"等电机"阶段**：硬件尚未到货，已给出**不用电机就能推进的学习清单**（见第十二节）。
> 4. **硬件采购调研已完成**（型号 / 渠道 / 闲鱼候选 / 必确认项，见第十三节）。
>
> **本次（2026-10-06）更新要点：**
> 5. **一体机已实购到手**：实物铭牌 **`ISS56C250CR11` / CANopen / 1.6 N·m / 2.4 A / 1.8° / SN 001389000013**
>    （见 13.1）。与旧记录「目标 2 N·m」低一档，不影响联调。
> 6. **电源与线材选型已定**：**24V / 5A（单台）**、电源线 **`RVV 2×1.5`**、冷压端子 **`E1508`**（见 13.5）。
> 7. **修正旧记录**：森创电源是**单独的 2 芯端子（`1=GND / 2=VCC`）**，**不是 9 芯插头 1/2 脚**
>    （原第十节第 8 条、13.4 已更正；此前 4.1/13.1 关于"9 芯 1/2 脚"的说法作废）。
>
> 旧版交接（2026-09-28）内容停留在「写第 2 站分册」；更早（2026-09-27）停留在「vcan 被 sudo 卡住」。
> 现在**总线已建好、程序已实跑、抓包证据已拿到、分册覆盖已到第 11 站**。

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

**当前阶段（2026-10-09 更新）：软件侧已全部落地、无阻塞，正在「等硬件到位」；硬件到位后立刻做**实物实验**（上板）。之后的两项增量已排定：**① CoE（CANopen over EtherCAT，知识迁移级）**、**② Modbus RTU IMU（IMU 接入 + Modbus 成果，一箭双雕）**——详见 §16.5。** 其中 ② 的**软件侧（无硬件可做部分）已于 2026-10-09 先行完成**：RS485 Modbus RTU 主站 `agv_modbus.h/.c` + 单测 + pty 仿真从站，`make test`/`ctest` 全过（见 §16.5 与实现册 `docs/MODBUS_IMU_IMPL_print.html`）。

- 背景：一体机 **`ISS56C250CR11` 已到手**（见 13.1），电源/线材已选型（见 13.5），但**尚未接线/通电**，
  真机此刻无法转；这**不影响软件推进**（本工程是从站设备工程，除「电机真转 + 真实位置回传」外，
  整条链路用 **vcan 虚拟总线 + `tools/sim_motors.py` 仿真从站**即可跑通）。详见**第十二节**。
- 并行事项 A（**已基本完成**）：**"没电机也能学/能做"的清单落地** —— vcan 全链 ✅、里程计 ✅、
  RT 抖动统计 ✅、异步统一日志 ✅、Qt HMI ✅、CMake + 交叉工具链 ✅（见第五、六节）。
- 并行事项 B：**硬件采购**已推进到「下单到手」；剩余工作 = 接线、上电前极性核查（见第十三节）。
- **近期已完成（2026-10-07）**：**下行控制闭环**（CiA402 状态机 + `0x6040` 使能时序 + `CO_NMT_sendCommand` 主站 Start +
  TPDO `0x50A/0x50B` 下发 `0x6040`+`0x60FF`）与**逆运动学**（`0x7000` cmd_vel → `0x60FF`，含双安全门限）**代码已落地**，
  使本工程从「监听器」升级为能控的「AGV 主控」。**vcan 闭环实跑验证已 PASS**（见第十五节、S11 §6）。
- 等候事项 C：**真机 500K 打通**（森创出厂 250K/站址 1 与工程 500K/node 10·11 冲突），
  以及「总线无第二节点 ACK」导致 `ERROR-WARNING` 的问题——必须等电机到位才能实测。
- **求职学习路线（2026-10-09 重排）见第十六节**——它是「为应聘而学」的总纲，与本文的技术增量路线（第十五节）并行、互为素材。
- **后续增量（2026-10-09 定）见 §16.5**：等硬件到位 → **实物实验**；之后 **① CoE**（知识迁移，不买硬件）、**② Modbus RTU IMU**（RS485 主站 → 新 OD `0x7010` → 与 `0x6FFF` 航向融合；配套学习册 `docs/MODBUS_LEARNING_MAP_print.html`、实现册 `docs/MODBUS_IMU_IMPL_print.html`）。**② 的软件侧已先行完成**。

> ⚠️ 旧交接的「写第 2 站分册」「当前处于等电机空档期」等表述**均已过期**：
> S1~S9 分册 + S11 分册 + 面试 QA + 各专题分册**全部**在 `docs/` 落地；代码侧亦已落地一批增量（见第五、六节）。
> **请以本文件第五、六、十五节与源码为准，不要沿用旧版结论。**
> 第七节保留为「第 2 站分册的原始提纲与取证数据」，作为**已归档参考**，不必再写一遍。

---

## 四、产出物体系与排版规范（分册必须照抄）

### 4.1 已有产出物（文档统一收在 `docs/`；模板见 `docs/`）

> 目录结构：`docs/` 放全部分册 HTML，`docs/hardware/` 放硬件资料（原理图 PDF）。
> 根目录只留 `Makefile / README.md / HANDOFF.md` 与源码、子目录。

| 文件 | 内容 |
|---|---|
| `docs/AGV_CANOPEN_LEARNING_MAP_print.html` | **主手册**（911 行）：§0 十站总览 + 第 0 站（工程全景）+ 第 1 站（CAN 总线 & SocketCAN） |
| `docs/AGV_CANOPEN_LEARNING_MAP_S1_WALKTHROUGH_print.html` | **第 1 站补充册**（304 行）：逐行慢读（链 A 建 socket / 链 B 收帧 / 发送重传） |
| `docs/AGV_CANOPEN_LEARNING_MAP_S2_WALKTHROUGH_print.html` | **第 2 站补充册**：报文层（COB-ID 总账 + NMT/心跳/SYNC/EMCY） |
| `docs/AGV_CANOPEN_LEARNING_MAP_S3_WALKTHROUGH_print.html` | **第 3 站**：对象字典 OD & SDO |
| `docs/AGV_CANOPEN_LEARNING_MAP_S4_WALKTHROUGH_print.html` | **第 4 站**：PDO 映射 & CiA402 |
| `docs/AGV_CANOPEN_LEARNING_MAP_S5_WALKTHROUGH_print.html` | **第 5 站**：启动链路走读（`CO_main_basic.c`） |
| `docs/AGV_CANOPEN_LEARNING_MAP_S6A_RT_THREAD_print.html` | **第 6 站 A**：实时性改造 —— RT 线程 |
| `docs/AGV_CANOPEN_LEARNING_MAP_S6B_RT_JITTER_print.html` | **第 6 站 B**：实时性改造 —— 抖动实测 |
| `docs/AGV_CANOPEN_LEARNING_MAP_S7_ODOMETRY_print.html` | **第 7 站**：里程计（差速运动学 → 位姿 x,y,θ） |
| `docs/AGV_CANOPEN_LEARNING_MAP_S8_QT_HMI_print.html` | **第 8 站**：Qt 上位机 —— 位姿显示与指令下发 |
| `docs/AGV_QT_CMDVEl_DOWNLINK_print.html` | **Qt 下行打通补充册**：经 CiA-309 网关 SDO 周期写 `0x7000` cmd_vel（滑条/停车/周期重发 + vcan 验证） |
| `docs/AGV_CANOPEN_LEARNING_MAP_S9_CROSS_COMPILE_DEPLOY_print.html` | **第 9 站**：交叉编译 & 部署到 i.MX6ULL（附：设备树与 CAN 驱动「够用层」） |
| `docs/AGV_CANOPEN_LEARNING_MAP_S10_DOWNLINK_CIA402_print.html` | **第 10 站**：下行控制闭环（CiA402 使能时序 + TPDO 下发 `0x6040`/`0x60FF` + NMT 主站） |
| `docs/AGV_CANOPEN_LEARNING_MAP_S11_INVERSE_KINEMATICS_print.html` | **第 11 站**：差速逆运动学（`0x7000` cmd_vel → `0x60FF`，含单位换算与双安全门限） |
| `docs/AGV_CANOPEN_INTERVIEW_QA_print.html` | **面试 QA 册**：面向简历/面试的问答整理 |
| `docs/AGV_CANOPEN_LEARNING_MAP_CANOPEN_MIND_print.html` | **CANopen 心智模型补充册**：一条总线、一张字典、几扇门 |
| `docs/MODBUS_LEARNING_MAP_print.html` | **Modbus 学习册**：学习深度裁定（应用/主站开发级，按岗位要求）+ 四区/功能码/RTU·TCP 帧/异常码/字节序/主站 + 落地到本项目（Modbus RTU IMU → OD `0x7010`） |
| `docs/MODBUS_IMU_IMPL_print.html` | **Modbus 实现册**：`agv_modbus` 主站从纯函数(CRC16/组帧/解帧/四种字序)到串口·RS485·事务层(超时重试/异常不重试/离线判定)的完整实现走读 + 单测(46 断言) + pty 端到端与故障注入验证（见 §16.5） |
| `docs/CONCURRENCY_QUEUE_PRODUCER_CONSUMER_print.html` | **并发分册 · 队列与生产者-消费者模型** |
| `docs/CONCURRENCY_THREADPOOL_print.html` | **并发分册 · 线程池**：原理、实现与嵌入式落地 |
| `docs/AGV_ASYNC_LOG_IMPL_print.html` | **日志分册 · AGV 异步日志改造**：代码梳理（精简版） |
| `docs/LOG_ERROR_HANDLING_print.html` | **日志分册 · 日志分级 + 错误处理**（知识讲解） |
| `docs/EMBEDDED_LINUX_APP_SELFCHECK_print.html` | **嵌入式 Linux 应用岗 · 系统编程自测清单（反向体检）** |
| `docs/TOOLCHAIN_BUILD_MAKEFILE_CMAKE_print.html` | **工具链分册 · 构建系统**：Makefile 精讲 + CMake 入门 |
| `docs/TOOLCHAIN_DEBUG_TRIO_print.html` | **工具链分册 · 调试三件套**：GDB / Valgrind / strace 按现象选工具 |
| `docs/P2_ENGINEERING_TEST_KNOWLEDGE_print.html` | **P2 工程化测试**：单元测试 + 故障注入 + 内存/线程检查（**绿色版式**，为 P3 起的参考模板） |
| `docs/MUSICPLAYER_CHAIN_QT_CPP_print.html` | **格式权威模板**（用户给的参考文件，不是本项目内容，只借排版，2629 行） |
| `HANDOFF.md` | 本文件（仍在根目录） |

> 说明：**S1~S9 分册 + S10（下行）+ S11（逆运动学） + 面试 QA + 各专题分册（并发 / 日志 / 工具链 / 心智模型 / 自检）均已落地**。
> 旧版交接记的「S2 未开始 / S3~S9 未开始 / 第 9 站尚无独立分册」**全部过期**；**分册覆盖已到第 11 站**。

### 4.2 主手册的十站地图（阶段 A 摸清现状 / B 做增量 / C 上位机与上板）

```
【阶段 A：摸清现状 —— 先跑起来，再彻底看懂】           （不改代码，只读）
 第 0 站  工程全景        ← 主手册已写                              ✅已完成
 第 1 站  CAN 总线 & SocketCAN   ← 主手册 + S1 分册                 ✅已完成
 第 2 站  CANopen 报文层（NMT / 心跳 / SYNC / COB-ID） ← 主手册 + S2 分册 ✅已完成
 第 3 站  对象字典 OD & SDO      ← S3 分册                          ✅已完成
 第 4 站  PDO 映射 & CiA402      ← S4 分册   ◀── 全工程的心脏       ✅已完成

【阶段 B：做出增量 —— 简历上的硬货】                    （开始改代码）
 第 5 站  启动链路走读（CO_main_basic.c）  ← S5 分册                ✅分册完成
 第 6 站  实时性改造（SCHED_FIFO + 抖动实测）← S6A/S6B 分册 + 代码已实现 ✅
 第 7 站  里程计（差速运动学 → 位姿 x,y,θ）← S7 分册 + 代码已实现    ✅

【阶段 C：上位机 & 上板 —— 闭环成"产品"】
 第 8 站  Qt 上位机（显示位姿 / 发指令）    ← S8 分册 + hmi/ 已实现  ✅ 画轨迹 + 发 NMT + **发 cmd_vel(0x7000)**
 第 9 站  交叉编译 & 部署到 imx6ull         ← S9 分册 + CMake/工具链已就绪 ✅分册完成
 第 10 站 下行控制闭环（CiA402 使能 + 下发）← S10 分册 + 代码已实现     ✅ 已通过 vcan 验证
 第 11 站 逆运动学（cmd_vel → 轮速/轮位）   ← S11 分册 + 代码已实现     ✅ 已通过 vcan 验证
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
| 编译（x86） | ✅ 完成 | Makefile/CMake 现为 **31 个 `.c`**（在旧 28 个基础上 +`agv_queue.c`/`agv_pool.c`/`agv_log.c`）；产出 `canopend`、`cocomm_cli` |
| vcan 虚拟总线 | ✅ 完成 | `can0` = vcan，状态 `<NOARP,UP,LOWER_UP> mtu 72` |
| 实跑 + 抓包 | ✅ 完成 | 抓到 `701 [1] 00` → `081 [8] 1081101400000000` → 每 100 ms 一帧 `080 [0]` |
| 主手册 §0 + 第 0 站 + 第 1 站 | ✅ 完成 | `docs/AGV_CANOPEN_LEARNING_MAP_print.html` |
| S1~S9 + S10/S11 分册 | ✅ 完成 | S1 总线 / S2 报文层 / S3 OD&SDO / S4 PDO&CiA402 / S5 启动链路 / S6A+B 实时性 / S7 里程计 / **S8 Qt** / **S9 交叉编译部署** / **S10 下行闭环** / **S11 逆运动学** |
| 面试 QA 册 + 专题分册 | ✅ 完成 | 面试 QA、CANopen 心智模型、并发（队列/线程池）、日志（异步/错误处理）、工具链（构建/调试）、嵌入式应用自检 |
| 工程目录整理 | ✅ 完成 | 分册 HTML 收入 `docs/`、原理图入 `docs/hardware/`；清构建产物（`.o`/`canopend`/`cocomm`/`__pycache__`） |
| **里程计正解（代码）** | ✅ 完成 | 第 7 站已落地：`app_programRt` 中点法积分 → `0x6FFF`（见 6.9） |
| **RT 定时抖动统计（代码）** | ✅ 完成 | `app_programRt` 窗口统计 + `[RT] tick ...` 打印（见 6.9） |
| **异步统一日志（代码）** | ✅ 完成 | `agv_log/queue/pool` + `log_printf` 三路派发（见 6.5） |
| **Qt HMI（轨迹 + cmd_vel 下发）** | ✅ 上下行打通 | 经网关轮询 `0x6FFF` 画轨迹 + 发 NMT；并 **SDO 周期写 `0x7000`(v,ω) 下发运动指令**（滑条 + 停车发零，见 6.6） |
| **仿真从站 `sim_motors.py`** | ✅ 完成 | 10 Hz 回放 `0x38A`/`0x38B`（见 6.7） |
| **CMake + ARM 交叉工具链** | ✅ 完成 | `CMakeLists.txt` / `toolchain-arm.cmake` / `deploy/canopend_arm`（见 6.8） |
| **下行控制闭环（CiA402 下发）** | ✅ 已通过 vcan 验证 | 第 8/10 站：`cia402_step` 按 `0x6040` 位序列驱动使能、`CO_NMT_sendCommand` 主动 Start、`0x1A00` 把 `0x6040`+`0x60FF` 经 TPDO `0x50A/0x50B` 下发（见第十五节 P0；2026-10-07 vcan 实测 PASS） |
| **逆运动学（v,ω → 轮速/轮位）** | ✅ 已通过 vcan 验证 | 第 11 站：上位机 SDO 写 `0x7000`(cmd_vel) → 逆解 `vR=v+ω·b/2`/`vL=v−ω·b/2` → 换算 counts/s → 安全门限 → 写 `0x60FF`（见 S11 分册、第十五节 P0.5；2026-10-07 vcan 实测 `(0.2,0)→6366` PASS） |
| **硬件（电机）采购** | 🔶 进行中 | 一体机 **`ISS56C250CR11` 已到手**（1.6 N·m，见 13.1）；电源/线材/端子**已选型**（24V/5A、RVV 2×1.5、E1508，见 13.5） |
| **真机 500K 联调** | ⬜ 阻塞 | 等硬件到位 → 实物实验；问题见第十二节末、第十节 |
| **Modbus 学习册（深度裁定）** | ✅ 完成 | `docs/MODBUS_LEARNING_MAP_print.html`（应用/主站开发级，见 §16.5） |
| **Modbus RTU 主站（软件侧）** | ✅ 完成（待硬件联调） | `agv_modbus.h/.c`：CRC16(0xA001)/组帧解帧/功能码 `0x03·0x04·0x06·0x10`/异常识别/超时重试/float32 四种字序/RS485 方向控制/t3.5 切帧；单测 46 断言 + pty 仿真从站 `tools/sim_imu_modbus.py`；`make test` 与 `ctest` 全过（见 §16.5、实现册） |
| **CoE（CANopen over EtherCAT）** | ⬜ 计划 | 知识迁移，**不做从站协议栈开发、不买硬件**；产出 CANopen↔CoE 对照表（见 §16.5） |
| **Modbus RTU IMU** | 🔶 软件侧完成，待硬件 | 主站 + 仿真从站已通（含 pty 端到端与故障注入）；剩：接真 IMU 调字序/方向 → 新 OD `0x7010` → 与 `0x6FFF` 航向融合（见 §16.5） |

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
    -DCO_CONFIG_STORAGE=0 -Wno-format -I. -ICANopenNode -Iapplication"
# SRCS = 28 个 .c，逐个：cc $CF -c "$s" -o "$(basename ${s%.c}).o"
cc -g -ggdb *.o -o canopend
```

> ⚠️ **P1 起禁止再加 `-DCO_CONFIG_HB_CONS=0`**：该宏会**整体覆盖** `CO_driver_target.h` 里的心跳配置（`#ifndef` 被跳过），从而**禁用 HBconsumer**，
> 使 P1 心跳监控失效。现工程由 [CO_driver_target.h:74-81](file:///home/zcy/Project/panasonic_canopen_agv/CO_driver_target.h#L74-L81) 统一配置（见 6.12）。

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

### 6.5 异步统一日志（代码，已落地）

- 目标：把 RT 线程里的 `printf` 从「同步写终端」改成「非阻塞入队 → 工作线程落盘」，避免终端 IO 抖动实时节拍。
- 三个文件构成分层：`agv_queue.c/h`（有界环形队列，容量取 2 的幂）→ `agv_pool.c/h`（常驻消费者线程池）→
  `agv_log.c/h`（分级日志 API + 宏）。反向不依赖，避免循环依赖（出处 [agv_log.h](file:///home/zcy/Project/panasonic_canopen_agv/agv_log.h#L11-L13)）。
- 对外 API（出处 [agv_log.h](file:///home/zcy/Project/panasonic_canopen_agv/agv_log.h#L47-L85)）：
  `agv_log_init(cap, nthreads)` / `agv_log_deinit()` / `agv_log_write(level,file,line,fmt,...)` /
  `agv_log_write_text(level,text)` / `agv_log_dropped()`；宏 `AGV_LOGD/I/W/E/F`（编译期门限 `AGV_LOG_MIN_LEVEL`，默认 INFO）。
- 接入主程序（出处 [CO_main_basic.c:366-377](file:///home/zcy/Project/panasonic_canopen_agv/CO_main_basic.c#L366-L377)）：
  `agv_log_init(256, 2)`（队列 256 + 2 工作线程）+ `atexit(agv_log_deinit)`；退出时再显式 `agv_log_deinit()`
  （[CO_main_basic.c:950](file:///home/zcy/Project/panasonic_canopen_agv/CO_main_basic.c#L950)）。
- `log_printf()` 是**三路派发适配器**（出处 [CO_main_basic.c:167-198](file:///home/zcy/Project/panasonic_canopen_agv/CO_main_basic.c#L167-L198)）：
  ① syslog（原有兼容）+ ② 异步日志 `agv_log_write_text()` + ③ 网关镜像 `CO_GTWA_log_print()`（`:185-198`）；
  级别映射见 `log_priority_to_level()`（[CO_main_basic.c:145-158](file:///home/zcy/Project/panasonic_canopen_agv/CO_main_basic.c#L145-L158)）。
- 关键设计：RT 线程只 `vsnprintf` + **非阻塞** `try_push`，队列满 / 锁被占则 `dropped++`，**绝不阻塞**。
- `.warn` 关闭顺序：必须先 `agv_queue_close()` 广播唤醒 worker，再 `join`；否则 `agv_pool_destroy` 会卡死
  （见 [agv_pool.h](file:///home/zcy/Project/panasonic_canopen_agv/agv_pool.h)）。
- 提交：`2c17acc feat: 实现异步统一日志模块，解耦RT线程与终端IO`。

### 6.6 Qt 上位机 · 轨迹显示 + cmd_vel 下发（✅ 上下行打通）

- `hmi/CommWorker`（`QThread`）经 **CiA-309 网关（`tcp-60000`）** 轮询自定义对象 `0x6FFF` 子索引 1/2/3
  （x/y/θ，float32），在 `TrajectoryView` 上画轨迹（出处 [commworker.h](file:///home/zcy/Project/panasonic_canopen_agv/hmi/commworker.h)）。
- **下行（本次新增）**：`CommWorker::sendWrite()`（[commworker.cpp:113-120](file:///home/zcy/Project/panasonic_canopen_agv/hmi/commworker.cpp#L113-L120)）
  按网关语法 `[<seq>] <node> w 0x7000 <sub> r32 <value>` 写 cmd_vel（子 1=v m/s、子 2=ω rad/s）。
  UI 是两根滑条（v ±0.5 m/s、ω ±1.0 rad/s）+「停车(发零)」按钮（[mainwindow.cpp:179-231](file:///home/zcy/Project/panasonic_canopen_agv/hmi/mainwindow.cpp#L179-L231)）。
- **周期重发（关键）**：主控有 **deadman 门限 500ms**（超时无新写入即把轮速清零），故不能只在滑条变动时写一次——
  `m_cmd` 定时器以 **150ms** 周期重发当前 (v,ω)（[commworker.cpp:106-111](file:///home/zcy/Project/panasonic_canopen_agv/hmi/commworker.cpp#L106-L111)）。
  滑条是**绝对量**（松手不停），停车须显式按「停车」或断开连接（断开时 `m_v=m_w=0` 且滑条归零）。
- **构建**：`qmake hmi.pro && make`（Qt 5.12.8，[hmi.pro](file:///home/zcy/Project/panasonic_canopen_agv/hmi/hmi.pro)）。
- **vcan 验证（2026-10-09 PASS）**：`canopend vcan0 -i 1 -c "tcp-60000"` + `tools/sim_motors.py vcan0 1 --both`（其状态字恒 `0x0237`=Operation enabled，满足"未使能不动"门限），
  用**逐字复刻 Qt `sendWrite()` 报文**的脚本连网关写 `0x7000`，抓 TPDO：
  - `v=+0.2, ω=0` → `0x50A`/`0x50B` 的 `0x60FF` = **6366 / 6366**（= 0.2/K，与逆解一致）；
  - `v=0, ω=0`（停车）→ `0x60FF` = **0 / 0**；
  - `v=0, ω=-0.5`（验证子 2）→ 右轮 `0x60FF` = **-2387**、左轮 = **+2387**（= ∓0.5·b/2/K，b=0.3m），左右反向 ⇒ 原地转，符号正确。
- 定位：上行可视化 + 下行指令条，至此「Qt → 网关 → 逆解 → 下发」**整条链在 HMI 侧闭环**（主控侧早已就绪，见第十五节 P0/P0.5）。

### 6.7 仿真从站 `tools/sim_motors.py`（零硬件回放）

- 用 Python 标准库 Raw CAN socket 模拟左右两个电机驱动器（出处 [sim_motors.py:31-46](file:///home/zcy/Project/panasonic_canopen_agv/tools/sim_motors.py#L31-L46)）。
- 开机先发 **NMT Start**（`0x000` 数据 `[0x01, node_id]`，[sim_motors.py:51](file:///home/zcy/Project/panasonic_canopen_agv/tools/sim_motors.py#L51)）
  让设备进 OPERATIONAL；再以 **10 Hz** 发 `0x38A`/`0x38B`，DLC=6，`struct.pack("<Hi", 0x0237, pos)`
  （状态字 + 实际位置，[sim_motors.py:79-81](file:///home/zcy/Project/panasonic_canopen_agv/tools/sim_motors.py#L79-L81)）。
- 三种模式：默认（右轮 1000 计数/秒、左轮不动）/ `--static`（都不动）/ `--both`（同速直行）。
- 价值：**没有任何电机也能跑通 RPDO→OD→里程计全链**。

### 6.8 CMake 构建 + ARM 交叉工具链（已就绪）

- `CMakeLists.txt`：现代 `target_*` 风格，`add_executable(canopend ...)` 列 **31 个源文件**（含 `agv_queue/pool/log.c`），
  `target_compile_definitions` 带 `CO_MULTIPLE_OD` / `CO_USE_APPLICATION=1` /
  `CO_CONFIG_STORAGE=0`（**注意：P1 起不可再加 `CO_CONFIG_HB_CONS=0`**，否则会覆盖 `CO_driver_target.h`
  的 `#ifndef` 而静默禁用心跳监控）；链接 `Threads::Threads` + `m`；另出 `cocomm`。见 [CMakeLists.txt](file:///home/zcy/Project/panasonic_canopen_agv/CMakeLists.txt)。
- `toolchain-arm.cmake`：`CMAKE_SYSTEM_NAME Linux` / `CMAKE_SYSTEM_PROCESSOR arm`，前缀 **Linaro 4.9.4**
  （`gcc-linaro-4.9.4-2017.01-x86_64_arm-linux-gnueabihf`）。见 [toolchain-arm.cmake](file:///home/zcy/Project/panasonic_canopen_agv/toolchain-arm.cmake)。
- 已产出 ARM 可执行 `deploy/canopend_arm`。
- `.note` 沙箱内仍沿用 **6.1 的「平铺编译」**；CMake 是给用户本机 / 交叉编译用的正规通路。

### 6.9 里程计正解 + RT 抖动统计（代码，已落地）

- 标定参数（出处 [CO_application.c:51-58](file:///home/zcy/Project/panasonic_canopen_agv/CO_application.c#L51-L58)）：
  `ODOM_WHEEL_R_M=0.050`、`ODOM_COUNT_PER_REV=10000`、`ODOM_WHEEL_BASE_M=0.300`、`ODOM_DIR_R/L`、`ODOM_K_PER_COUNT=2πr/N`。
- 收帧钩子 `H6064_write()`（[CO_application.c:75-92](file:///home/zcy/Project/panasonic_canopen_agv/CO_application.c#L75-L92)）：
  按 COB-ID `0x038A`→`odom_posR_raw`、`0x038B`→`odom_posL_raw` 分流，置 `odom_new=true`（**只存值，不积分**）。
- RT 主逻辑 `app_programRt()`（[CO_application.c:227-329](file:///home/zcy/Project/panasonic_canopen_agv/CO_application.c#L227-L329)）：
  - **抖动窗口统计**：阈值 `RT_JITTER_OVER_US=200`、窗口 `RT_STAT_TICKS=1000`，到点打印 `[RT] tick ...`。
  - **里程计积分**：`dCntR/dCntL` 用无符号相减处理回绕 → `dR/dL` → `d=(dR+dL)/2`、`dθ=(dR−dL)/b`；
    中点法 `θmid=θ+dθ/2` → `x+=d·cos θmid`、`y+=d·sin θmid`、`θ+=dθ`；`CO_LOCK_OD` 下写入 `OD_RAM.x6FFF_agvOdometry[0..2]`，每 1000 拍打印 `[ODOM]`。
- 提交：`63e58fa feat: 添加AGV里程计上位机及CANopen实时节拍统计`。

### 6.10 多线程 `CO_2nd` 修复（遗留 #4 已修）

- `rt_thread()` 现**同时**处理两套 OD（出处 [CO_main_basic.c:959-967](file:///home/zcy/Project/panasonic_canopen_agv/CO_main_basic.c#L959-L967)）：
  `CO_epoll_processRT(&epRT, CO, true)` + `CO_epoll_processRT(&epRT, CO_2nd, true)`。
- 旧版只处理 `CO`，左电机（第二套 OD）的 RT 侧处理会丢；现已补齐。
- `.note` 主循环里的 `CO_epoll_processMain(&epMain, CO_2nd, ...)`（[CO_main_basic.c:876](file:///home/zcy/Project/panasonic_canopen_agv/CO_main_basic.c#L876)）
  **仍被注释** —— 见第十节 #5。

### 6.11 下行控制闭环 + 逆运动学（代码已落地 + vcan 闭环验证 PASS；规格 = S10/S11 分册）

> **2026-10-07 vcan 零硬件闭环实测（PASS）**：上位机 SDO 周期写 `0x7000`(v=0.2, ω=0) 4s →
> 总线上抓到 TPDO `0x50A`(右轮)/`0x50B`(左轮)，两者末帧载荷均为 `0f00de180000`：
> `0x6040=0x000F`（Operation enabled）、`0x60FF=0x000018DE=6366`（≈`0.2/3.14159e-5`，与逆解预测完全一致）；
> SDO 响应 `0x581` 回 `6000700100000000`（cmd `0x60` 下载成功 + idx `0x7000`/sub `0x01`）。
> ⚠️ 左轮帧数远少于右轮（2 vs 32）：右轮 `OD.c` 的 `0x1800.eventTimer=0x0100`(≈256ms 周期兜底)，左轮 `OD_2nd.c` 的 `eventTimer=0x0000`(纯事件驱动)——
> 功能正确，但左右下行刷新特性不一致，建议把左轮 `eventTimer` 对齐为 `0x0100`（小改进项，见第十五节 P0）。

**A. 下行控制闭环（第 10 站）**

- **CiA402 使能时序** `cia402_step(sw)`（[CO_application.c:187-194](file:///home/zcy/Project/panasonic_canopen_agv/CO_application.c#L187-L194)）：
  按右/左轮状态字推控制字——`(sw&0x4F)==0x40`→`0x0006`；`(sw&0x6F)==0x21`→`0x0007`；`==0x23`→`0x000F`；`==0x27`→保持 `0x000F`；其它→`0x0006`。
- **状态字来源** `H6041_write()`（`0x6041` 扩展写钩子，[CO_application.c:135-151](file:///home/zcy/Project/panasonic_canopen_agv/CO_application.c#L135-L151)）：
  状态字经 RPDO（映射 `0x60410010`，COB-ID `0x038A/0x038B`）到达后，分流缓存到 `dl_swR/dl_swL`，供 `cia402_step` 使用（`0x6041` 在两本 OD 里 `dataOrig=NULL`，必须靠钩子接住）。
- **下发通道**：右/左两套 OD 的 `0x1A00` TPDO 映射配成 `0x6040`（16bit）+`0x60FF`（32bit），经 `0x50A`/`0x50B` 下发（[CO_application.c:338-364](file:///home/zcy/Project/panasonic_canopen_agv/CO_application.c#L338-L364)）。
  模式默认 `0x6060=3`（Profile Velocity）。
- **主站 Start**：`CO_NMT_sendCommand(co->NMT, CO_NMT_ENTER_OPERATIONAL, DL_NODE_R/L)`（[CO_application.c:428-429](file:///home/zcy/Project/panasonic_canopen_agv/CO_application.c#L428-L429)）。
- **触发**：`0x6040`/`0x60FF` 均挂扩展启用 `flagsPDO`，写值变化即 `OD_requestTPDO`。

**B. 逆运动学 + 运动下发（第 11 站）**

- **入口对象** `0x7000 agvCmdVel`（float32×2，子 1=`v` m/s、子 2=`ω` rad/s，`ODA_SDO_RW`）：
  追加在 ODList **末尾 `list[158]`**（> `0x6FFF`，保持 `OD_find` 二分所需升序），见 [OD.c:2144-2145](file:///home/zcy/Project/panasonic_canopen_agv/application/OD.c#L2144-L2145)。
- **RT 落地**（[CO_application.c:502-540](file:///home/zcy/Project/panasonic_canopen_agv/CO_application.c#L502-L540)）：
  读 cmd_vel → 逆解 `vR=v+ω·b/2`、`vL=v−ω·b/2` → `counts/s = ODOM_DIR_* · v / ODOM_K_PER_COUNT` → 门限 → 写 `0x60FF` 并触发 TPDO。
- **双安全门限（S11 §5.4）**：① **未使能不动**（两轮 `0x6041` 非 `0x27` 态则轮速清零）；
  ② **失联停车 deadman**（[CO_application.c:216-222](file:///home/zcy/Project/panasonic_canopen_agv/CO_application.c#L216-L222) 写钩子 `H7000_write` 打时间戳，超 `CMD_TIMEOUT_TICKS=500`(≈500ms) 无新指令即清零）。
- **实现要点**：写钩子**不得自行加锁**（SDO 服务端已在 [CO_SDOserver.c:555-558](file:///home/zcy/Project/panasonic_canopen_agv/CANopenNode/301/CO_SDOserver.c#L555-L558) 外层持 `CO_LOCK_OD`，再加锁会死锁）。
- **编译**：`make` 已通过（平铺编译，见 6.1）。
- **待办**：~~vcan 闭环实跑验证（S11 §6）~~ **已完成（2026-10-07 PASS）**；正式版需用 CANopenEditor 把 `0x7000` 写进 EDS 再重生成 OD，避免手工改动被覆盖。

### 6.12 心跳容错（P1 已落地 + vcan 验证 PASS；正面解法 = 遗留 #9）

> **2026-10-07 vcan 零硬件实测（PASS）**：`sim_motors.py` 周期发心跳 → 停发 → 约 1s 后日志出现
> `[E] CO_application.c:534 [HB] heartbeat LOST: node10=3 node11=3`（3=`TIMEOUT`）+ 库自动 EMCY `0x8130`(errorBit `0x1B`)；
> 重启 sim 恢复心跳 → 日志出现 `[HB] heartbeat OK: node10=2 node11=2`（2=`ACTIVE`）+ EMCY `errorCode=0x0000` 清除。
> **「收到过心跳→ACTIVE→停发→TIMEOUT→EMCY+强制零速→恢复→ACTIVE」全链路闭环验证通过。**

- **做什么**：把现成的 HBconsumer（8 槽，`OD_CNT_ARR_1016=8`）真正用于监控两个驱动器；
  任一所监控从站**心跳超时 → 判掉线 → 库自动报 EMCY + 应用侧强制零速**；心跳恢复 → 自动回 `ACTIVE`。
- **监控槽配置** `0x1016 consumerHeartbeatTime`（[OD.c:31-35](file:///home/zcy/Project/panasonic_canopen_agv/application/OD.c#L31-L35)）：
  `sub0=0x08`（槽数），`sub1=0x000A03E8`（`(10<<16)|1000` → 监控 node10=右轮，超时 1000ms），
  `sub2=0x000B03E8`（node11=左轮），其余槽 `0`（禁用）。`0x1017` 保持 `0x0000`（未启用自身心跳）。
- **关键机制（必须知道）**：`CO_HBconsumer_process()` 只在 `HBstate==ACTIVE` 时判超时（[CO_HBconsumer.c:417-433](file:///home/zcy/Project/panasonic_canopen_agv/CANopenNode/301/CO_HBconsumer.c#L417-L433)），
  且 `HBstate` 仅在**收到过至少一次心跳**后由 RX 回调置 `ACTIVE`（[CO_HBconsumer.c:408](file:///home/zcy/Project/panasonic_canopen_agv/CANopenNode/301/CO_HBconsumer.c#L408)）→
  **从未上线的从站（`UNKNOWN`）不会报超时**（CANopenNode 设计如此）；本工程由 `dl_sw` 门限互补覆盖"上电即没响应"场景。
- **RT 拍降级**（[CO_application.c:525-544](file:///home/zcy/Project/panasonic_canopen_agv/CO_application.c#L525-L544)）：
  每拍 `CO_HBconsumer_getState(co->HBcons, 0/1)` 读两槽状态（`idx` 为 **0 基数组下标**，槽 1→idx0、槽 2→idx1），
  任一 `TIMEOUT` 则 `hb_lost=true` → 与既有 `dl_sw`/`cmd_timeout` 合并进安全门限，**轮速强制清零**；
  仅在跃迁到/退出 `TIMEOUT` 时打一行日志（避免刷屏）。
- **EMCY**：由 HBconsumer **库内自动上报** `errorCode=0x8130`（`CO_EMC_HEARTBEAT`），应用层**无需**手写；恢复时库自动 `CO_errorReset`。
- **编译开关**（[CO_driver_target.h:74-81](file:///home/zcy/Project/panasonic_canopen_agv/CO_driver_target.h#L74-L81)）：
  `CO_CONFIG_HB_CONS = ENABLE | CALLBACK_CHANGE | QUERY_FUNCT | ...`——**新增 `QUERY_FUNCT` 以启用 `CO_HBconsumer_getState()`**；
  ⚠️ 不要再在 Makefile / 命令行加 `-DCO_CONFIG_HB_CONS=0`（会整体覆盖、禁用本功能，见 6.1）。
- **顺手修正**：左轮 `OD_2nd.c` 的 `0x1800.eventTimer` `0x0000 → 0x0100`（对齐右轮，消除下行帧数不对称）。
- **零硬件验证**：`tools/sim_motors.py` 增补**周期心跳帧**（COB-ID `0x700+nodeId`，载荷=NMT 状态 `0x05`）与 `--drop-hb-r`/`--drop-hb-l` 开关（停发某一路心跳模拟掉线）；
  另修 `argv[2]` 解析（仅数字才当 node-id，避免 `--static` 被 `int()` 崩溃）。
- **待办**：真机联调时确认两驱动器**实际心跳周期**并据此调 `0x1016` 超时值（应 ≥ 3× 心跳周期）；CANopenEditor 把 `0x1016` 写入 EDS 正式化。

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
4. （代码层）~~多线程 `CO_2nd` RT 路径丢失~~ → **已修**：`rt_thread()` 现同时处理 `CO` 与 `CO_2nd`
   （[CO_main_basic.c:959-967](file:///home/zcy/Project/panasonic_canopen_agv/CO_main_basic.c#L959-L967)，见 6.10）。
5. （代码层，未修）`CO_2nd` 的**主循环**完整 `CO_process` 仍未执行：[CO_main_basic.c:876](file:///home/zcy/Project/panasonic_canopen_agv/CO_main_basic.c#L876) 被注释。
   —— 影响：左电机在**主循环侧**的 NMT/SDO/EMCY 等非 RT 处理缺失。第 5 站读启动链路时会遇到，**这也是可做增量的点**。
6. 目录下残留 `CANopenNode-1191979f.../`（重复目录），沙箱拒绝删除，可忽略或用户手动 `rm -rf`。
7. （已办）HMI 默认板卡 IP 为 `169.254.86.72`（`hmi/mainwindow.cpp:144`）；**2026-10-09 已重新编译
   （`qmake hmi.pro && make`）**，并补上**经网关 SDO 写 `0x7000` cmd_vel** 的下行指令条，见 6.6 与
   `docs/AGV_QT_CMDVEl_DOWNLINK_print.html`；vcan（连 `127.0.0.1` 网关）验证 PASS。真机/带屏点滑条即可复现。
8. ~~森创 9 芯插头 1/2 脚是否为电源~~ → **已确认并更正**：电源是**单独的 2 芯端子（`1=GND / 2=VCC`）**，
   9/10 芯是 I/O（见 13.1）。仍待办：到货后**拍端子丝印照片核对**，并向卖家**索要配套对接插头（线束）**。
9. ~~（代码层，未用）心跳消费者 **HBconsumer 未被真正用于监控驱动器**~~ → **已解决（2026-10-07，P1）**：
   `0x1016` 已配 node10/node11 两槽（[OD.c:31-35](file:///home/zcy/Project/panasonic_canopen_agv/application/OD.c#L31-L35)），
   RT 拍读 `getState` 做安全降级（[CO_application.c:525-544](file:///home/zcy/Project/panasonic_canopen_agv/CO_application.c#L525-L544)），
   并修 `Makefile` 去掉 `-DCO_CONFIG_HB_CONS=0` 覆盖陷阱。**vcan 实测 PASS**（超时降级 + 恢复重连），详见 **6.12**。
   注：`CO_main_basic.c:761-763` 仅注册索引 0 的 **NMT 变化回调**（用于日志），本方案改用**轮询 `getState`** 实现，未改动该回调。
10. （工作区，未提交）`CMakeLists.txt` / `toolchain-arm.cmake` / `docs/AGV_CANOPEN_LEARNING_MAP_S9_...html` /
    `docs/TOOLCHAIN_*` 为 **untracked（`??`）**，`docs/*` 若干为**已暂存重命名（`R`）**，
    `HANDOFF.md` / `.gitignore` / `cocomm/cocomm.c` 为**已修改（`M`）** → **待一次整理提交**。
11. （工作区）构建产物（`deploy/canopend_arm`、`hmi/` 内的已编译可执行等）建议并入 `.gitignore`，勿提交。

---

## 十二、等电机期间：不用电机就能学/能做的事（2026-10-05 答复；**2026-10-07：12.1 已基本落地**）

**核心判断：本工程是 CANopen 从站（设备）工程，除「电机真转 + 真实位置回传」外，
整条软件链路用 vcan 虚拟总线即可跑通、可学、可写进简历。**

> ✅ **2026-10-07 复盘：下面 12.1 的清单已基本做完**（vcan 全链 / 里程计 / RT 抖动 / 异步日志 / Qt HMI / CMake 工具链
> 均已落地，见第五、六节）。**下行闭环与逆运动学也已落地并通过 vcan 闭环验证（2026-10-07 PASS）**，剩余未做只有 **心跳监控（P1）**（见第十五节）。

### 12.1 零硬件就能做（**已基本落地**，交叉引用第六节）

1. **vcan 上把整条链跑通**（最核心）✅。`canopend` 的 `-c` 网管接口支持 `tcp-<port>` / `local-<file>`，
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
3. **里程计（第 7 站）纯数学，零硬件** ✅ **已落地**：自定义对象 `0x6FFF`(float32×3) 就是给它留的，见
   [OD.c:281-283](file:///home/zcy/Project/panasonic_canopen_agv/application/OD.c#L281-L283)（实现见 6.9）。
4. **Qt HMI（第 8 站）提前做** 🔶 **半成品**：PC 上编译 `hmi/`，把 IP 改成 `127.0.0.1` 连 vcan 上的网关，
   验证「UI → 网关 → CAN」全链（默认板卡 IP 见 [mainwindow.cpp:144](file:///home/zcy/Project/panasonic_canopen_agv/hmi/mainwindow.cpp#L144)）。
   现状：画轨迹已通，**运动下发待补**（见 6.6）。
5. **理论分册** ✅：`S1~S9` + `S10/S11` 分册 + 各专题（并发/日志/工具链/心智模型/自检）**已全部落地**，可直接精读。
6. **仿真从站 `tools/sim_motors.py`**（新增）✅：10 Hz 回放 `0x38A`/`0x38B`，零硬件跑通 RPDO→OD→里程计（见 6.7）。

### 12.2 必须等电机（真机）才能做

| 能力 | 原因 |
|---|---|
| 真实位置回传 | `0x6064` / `0x6041` 需电机侧实际值，vcan 只能回填假值 |
| 电机实际转动 | 使能 / 电流环要真机（本工程不研究算法，但要看得到动） |
| 真机总线 ACK | 500K 波特率 + 第二节点应答；**vcan 没有 ACK**，所以现在遇不到 `ERROR-WARNING` 刷屏 |

---

## 十三、硬件采购调研（2026-10-05）

**目标硬件**：森创（syn-tron / 北京和利时）**ISS56C250DR12**，CANopen 闭环步进一体机，**2 N·m**。
> ⚠️ **实际购买的是低一档的 `ISS56C250CR11`（1.6 N·m，¥80，闲鱼 id=1089386761744）**，已到手，见 13.1。

### 13.1 型号解码 & 出厂参数（真机联调关键）

- `ISS` = 一体集成式步进伺服；机座号(42/56/57/60/86)；`C` = 通讯格式(C=CANopen, R=RS485)；
  `2` = 相数；`50` = 齿数；机身长度字母；后缀 `R00/R11/R12`。
- **出厂默认波特率 250K、站址 1**；改波特率 = SDO `2000h` 子索引 4，改站址 = `2000h` 子索引 3（1-127），
  **改动需重新上电**。← 这与本工程 **500K / node 10·11** 冲突，是**真机联调第一坑**。

**实物铭牌实测（2026-10-06 拍照确认）** —— `ISS56C250CR11`，标签 `CANopen 1.6Nm 2.4A 1.8° 2.1`，`SN: 001389000013`：

| 铭牌字段 | 解读 |
|---|---|
| `CANopen` | 通讯格式 = CANopen（**不是 RS485**） |
| `1.6 N·m` | 保持转矩（比计划的 2 N·m 低一档） |
| **`2.4 A`** | **额定相电流 —— 决定电源电流的关键值**（与 56S 规格表 `56SBYG250CK/DK`=2.4A 一致） |
| `1.8°` | 步距角 = 200 步/转（**注意 ≠ 编码器计数**，`ODOM_COUNT_PER_REV=10000` 是编码器计数） |
| `2.1` | 设计版本号 |

> ⚠️ 铭牌上**没有额定电压，只有电流**。电压以官方手册为准：ISS57-C 手册 `16 / 24 / 48V（不可超 48V）`，
> 旧手册 `ISS57XXX = 24~60V` → 取交集，**推荐 24V**。

**森创 ISS 端子定义（官方手册口径，以实物丝印为准）**

| 端口 | 芯数 | 定义 |
|---|---|---|
| 电源 | **2 芯** | `1 = GND（电源负端）`、`2 = VCC（电源正端）` |
| CAN | 4 芯 | `1 = CANL`、`2 = NC/5V`、`3 = CANH`、`4 = COM` |
| I/O | 10 芯 | `DI1± / DI2± / DI3± / DI4± / DO1±`（原点 / 正限位 / 负限位 / 未定义 / 报警） |
| 调试串口 | 4 芯 | `RXD / TXD / COM / 5V` |

- **拨码**：`SW1~SW4` = 从机地址（出厂 01，寄存器 `200Bh`）；`SW5` = CAN 波特率（出厂 500Kbps，ON=1000Kbps，寄存器 `200Ch`）；**改拨码后重新上电生效**。
- **容量口径（官方原文）**：单台容量一般 ≤ 100W；**多台时应各台单独引线到电源，避免链式供电回路**；**电源切勿反接**。
- **无内置泄放回路**：回馈能量大的场合需用户自行外加泄放模块。
- **线径要求**：9 芯端子用 `0.2~0.5mm²`；**电源用单独的 2 芯端子，导线 `1~1.5mm²`**。

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

> ✅ **用户实际下单：`ISS56C250CR11`（CANopen 1.6 N·m，¥80）**，闲鱼 id=**1089386761744**
> （与上表 id=1064824113999 是不同卖家、同型号）。**已到手**，实物铭牌见 13.1。

### 13.4 下单前必确认

- 必须是 **CANopen**（不是只 RS485 / Modbus）；
- 机座 **57 / 56**，扭矩 **≥ 2 N·m**；
- 向卖家**索要配套对接插头（线束）+ 上位机软件 + 手册**。
- ~~确认 9 芯插头 1/2 脚是否为电源~~ → **已作废**：电源是**单独的 2 芯端子**，9/10 芯是 I/O（见 13.1）。

### 13.5 电源与线材选型（2026-10-06 已定，可直接下单）

**电源电流怎么算**：额定相电流 2.4A/相 × 两相同时通电 = 4.8A → 驱动器斩波后母线平均 ≈ 0.6~0.7 × 4.8 ≈ **3.0~3.4A**；
官方口径「单台 ≤ 100W」÷ 24V ≈ **4.2A**。两者都指向 **4A 上下**。

| 带几台 | 推荐电源 | 说明 |
|---|---|---|
| 一台 | **24V / 5A（120W）** | 有约 20% 余量，选它 |
| 两台 | **24V / 10A（240W）** | 各台单独引线到电源，不要链式供电 |
| （用户提过的）24V / 14.6A（350W） | 过剩但能用 | 够带 3 台 |

**电源线（用户已选定的商品）**：`国标 2 芯 RVV 电缆线`，截面积 **1.5 平方毫米**（¥3.62/米）。

- RVV = 铜芯 PVC 绝缘 + PVC 护套**双芯软护套线**（俗称"红黑双并线"），**一根线两芯**，红黑一次搞定。
- 线径依据：官方要求 **1~1.5mm²**；1.5mm² 载流约 15A，4A 场景绰绰有余，压降更小。
- **长度**：建议买 **3~5 米**（两根电源线各留约 20cm 裁剪余量）。
- 若改用**特软耐高温硅胶线**（单芯线）：要**红、黑各买一根**，截面选 **16AWG ≈ 1.31mm²**
  （12AWG=3.31mm² **太粗**，13AWG=2.62、14AWG=2.08 偏粗，16AWG 才落在 1~1.5mm² 内）。

**冷压端子**：配 **`E1508`**（配 1.5mm²）；若用 1.0mm² 线则用 `E1008`。需**冷压钳 + 剥线钳**。

**接线法（开关电源 ↔ 一体机）**：

- 开关电源侧：交流输入 `L / N / PE`；直流输出 `+V` / `−V`（S-120-24 / S-350-24 这类）。
- 一体机侧：**`2 = VCC` ← 接 `+V`**；**`1 = GND` ← 接 `−V`**。
- 流程：断电接线 → 压管型端子 → 上螺丝 → **上电前用万用表量极性**。
- 四个坑：① **切勿反接**（反接直接损坏）；② 两台电机**各自引线到电源**，不要一台串一台；
  ③ 电源线**远离 CAN 信号线**（需靠近就 90° 交叉）；④ **无内置泄放回路**，回馈能量大要外加泄放模块。
- 建议在 `+V` 上串一个熔断器。
- ⚠️ `1=GND / 2=VCC` 以**实物丝印**为准（到货后拍端子照片核对）。

**禁用**：杜邦线/细信号线、单股硬线、**CAN 的红蓝屏蔽双绞线**（0.2~0.5mm²，不能当电源线）。

---

## 十四、给下一个对话的一句话摘要

> 用户要在**别人做好的现成工程**上做嵌入式 CANopen/AGV 项目（不自研、可买便宜硬件、**不碰电机算法**、
> 要能写进简历、**分册推进、输出凝练**）。
> **新增硬要求（2026-10-05）**：讲代码**以函数为单位**（干什么/被谁调/调了谁）；**必须标 `文件:行号` +
> 必须配流程图**；变量**在哪出现就在那说明**；讲清**整个工程的运行链路**；**不用 `AskUserQuestion`**；**中文回复**。
> 底座 = **松下 CANopenLinux sample1（実験用 AGV）**，代码在 `/home/zcy/Project/panasonic_canopen_agv/`，
> 已用"平铺编译"编出 `canopend`（521712 B），已在 vcan `can0` 上**实跑并抓到 `701/081/080` 三类帧**。
> 文档 = **主手册 911 行 + S1~S11 分册 + 面试 QA + 各专题分册（并发/日志/工具链/心智模型/自检）**；
> **十一站现已覆盖到 S11**（旧记录「第 9 站无分册」「十站全覆盖」已过期；新增 `docs/AGV_CANOPEN_LEARNING_MAP_S11_INVERSE_KINEMATICS_print.html`）。
> 代码侧已落地一批增量：**里程计正解**（`0x6FFF`）、**RT 抖动统计**、**异步统一日志**（`agv_log/queue/pool`）、
> **仿真从站 `tools/sim_motors.py`**、**CMake + ARM 交叉工具链**（`deploy/canopend_arm`）、**Qt HMI 画轨迹**、
> **下行控制闭环**（`0x6040` 使能时序 + TPDO `0x50A/0x50B` 下发 + `CO_NMT_sendCommand`）、
> **逆运动学**（`0x7000` cmd_vel → `0x60FF` 目标速度，含双安全门限）（见第五、六节）。
> **当前阶段：一体机已到手（`ISS56C250CR11`，1.6 N·m，见 13.1），电源与线材已选型（24V/5A、RVV 2×1.5、E1508，见 13.5）**；
> 软件侧**零硬件仍可推进**（vcan 全链 / sim_motors 回放 / 里程计 / Qt HMI，详见第十二节）。
> **下行闭环 + 逆运动学已在代码侧落地（P0/P0.5），并已通过 vcan 闭环实跑验证（2026-10-07 PASS）**（见第十五节 P0.5 与 S11 §6）；
> 心跳消费者未用于监控（→P1）。
> 接好线后做**真机 500K 联调**（森创出厂 250K/站址 1 需改，且有"总线无第二节点 ACK"问题）。
> 必须如实告知：这份 sample 应用层被精简为"收并打印左右电机位置"，是真实可跑的设备层骨架，
> 完整导航/运动控制需作为增量补齐（这正是简历价值所在）。

---

## 十五、下一步增量路线（零硬件可做，按简历含金量排序）

> 背景：工程原先是"**能看**的从站骨架（收 RPDO → 算里程计 → 画轨迹），**不能控**"；
> 现已通过 P0/P0.5 补上下行与逆运动学，并**于 2026-10-07 通过 vcan 闭环实跑验证（PASS）**。
> 以下四条是**在无硬件**前提下含金量最高的增量。共同点：**都能用 vcan + `sim_motors.py` 验证**。

### P0 · 下行控制闭环（✅ 已通过 vcan 验证）

> **vcan 实测（2026-10-07 PASS）**：`0x7000` 写 (v=0.2, ω=0) → TPDO `0x50A/0x50B` 末帧均 `0x6040=0x000F` + `0x60FF=6366`。
> **遗留小改进项**：左右 TPDO 刷新特性不一致——右轮 `OD.c:123 eventTimer=0x0100`（≈256ms 周期兜底），
> 左轮 `OD_2nd.c:97 eventTimer=0x0000`（纯事件驱动，故仅 2 帧）。功能正确，建议把左轮对齐为 `0x0100`。

> **落地情况（2026-10-07）**：已实现「`cia402_step` 按 `0x6041` 状态字推 `0x6040` 控制字 →
> `CO_NMT_sendCommand(ENTER_OPERATIONAL)` 主动 Start → `0x1A00` 映射 `0x6040`+`0x60FF` 经 TPDO `0x50A/0x50B` 下发」。
> 实施细节：**走 TPDO 下行（非 RPDO）**；模式默认 **`0x6060=3`（Profile Velocity）**，故下发对象是 `0x60FF` 目标速度而非 `0x607A` 目标位置。

- **做什么**：
  1. **CiA402 状态机** [CO_application.c:187-194](file:///home/zcy/Project/panasonic_canopen_agv/CO_application.c#L187-L194)：`cia402_step(sw)` 按 `0x6040` 的位序列驱动 `0x6041` 状态字走
     「Switch on disabled → Ready to switch on → Switched on → Operation enabled」，未知态一律退 `0x0006` 安全态。模式默认 `0x6060=3`。
  2. **下发通道** [CO_application.c:338-364](file:///home/zcy/Project/panasonic_canopen_agv/CO_application.c#L338-L364)：右/左两套 OD 的 TPDO 映射（`0x1A00`）配成 `0x6040`（控制字）+`0x60FF`（目标速度），经 `0x50A`/`0x50B` 下发。
  3. **主站控制** [CO_application.c:428-429](file:///home/zcy/Project/panasonic_canopen_agv/CO_application.c#L428-L429)：用 `CO_NMT_sendCommand(co->NMT, CO_NMT_ENTER_OPERATIONAL, DL_NODE_R/L)` 主动 Start 两从站。
- **为什么值钱**：这是 AGV **主控**的本职——面试会问「你怎么让电机动起来、怎么做使能时序、怎么做模式切换」。
  做出来，本项目就从"监听器"升级为"**能下发指令的主站**"，简历从"读过 CANopen"跃升为"**做过 CANopen 控制**"。
- **零硬件怎么验证**：写一个**反转版 `sim_motors.py`**（新增脚本：收 `0x38A`/`0x38B` 指令，按 CiA402 状态机回填状态字，
  并让位置随目标值"爬"）——即可在 vcan 上闭环验证使能时序与位置下达。
- **关联**：逆运动学 P0.5 会喂给这里的 `0x60FF`。

### P0.5 · 正逆运动学闭环（✅ 已通过 vcan 验证）

> **vcan 实测（2026-10-07 PASS）**：`0x7000` 写 (v=0.2, ω=0) → TPDO `0x50A/0x50B` 的 `0x60FF=6366`
>（= `0.2/3.14159e-5`，与逆解预测完全一致），两轮同速即直行，符合预期。
> **落地情况（2026-10-07）**：新增 OD 对象 **`0x7000 agvCmdVel`**（float32×2，子 1=`v` m/s、子 2=`ω` rad/s，`ODA_SDO_RW`，追加在 ODList 末尾 `list[158]`，升序合法）；
> RT 拍（[CO_application.c:502-540](file:///home/zcy/Project/panasonic_canopen_agv/CO_application.c#L502-L540)）执行「读 cmd_vel → 逆解 → 换算 → 安全门限 → 写 `0x60FF` 并触发 TPDO」。
> 双安全门限：① **未使能不动**（两轮 `0x6041` 非 Operation enabled 则轮速强制 0）；② **失联停车 deadman**
> （[CO_application.c:216-222](file:///home/zcy/Project/panasonic_canopen_agv/CO_application.c#L216-L222) `H7000_write` 写钩子在每次 SDO 写 `0x7000` 时打时间戳，超 500ms 无新指令即清零）。
> ⚠️ 单位换算沿用里程计标定常数（`ODOM_WHEEL_BASE_M`/`ODOM_K_PER_COUNT`/`ODOM_DIR_R/L`），**真机前须按实测标定**。规格见 `docs/AGV_CANOPEN_LEARNING_MAP_S11_INVERSE_KINEMATICS_print.html`。

- **做什么**：在已落地的**正解**（两轮计数 → x,y,θ，见 6.9）之上补**逆解**：
  给定期望线速度 `v` 与角速度 `ω`（cmd_vel），算出左右轮目标轮面线速度 = `v ± ω·b/2`，换算 counts/s 后下发到 P0 的 `0x60FF`。
- **为什么值钱**：**AMR/AGV 岗位的"标准考点"**——差速底盘的 `v/ω → 轮速` 换算、轮距 b、`(v±ωb/2)`。
  能讲清"上位机发 cmd_vel，底盘怎么变成左右轮动作"，是 AGV 控制岗的核心竞争力。
- **零硬件验证（✅ 已完成）**：`0x7000` 写 (v=0.2, ω=0) → 抓 `0x50A` 的 `0x60FF` ≈ `0.2/3.14159e-5` ≈ 6366 → `sim_motors.py` 回放 → `0x6FFF` 应呈直线；
  换 (v=0, ω=1) → 应原地转。详见 S11 §6。

### P1 · NMT 与心跳容错（工程健壮性）— ✅ 已落地 + vcan 验证 PASS（2026-10-07）

> **落地情况（2026-10-07）**：`0x1016` 配 node10/node11 两槽（`OD.c:31-35`）→ RT 拍 `CO_HBconsumer_getState`
> 读状态做安全降级（`CO_application.c:525-544`）→ 去掉 `Makefile` 的 `-DCO_CONFIG_HB_CONS=0` 覆盖陷阱，
> `CO_driver_target.h` 追加 `QUERY_FUNCT`。**vcan 实测 PASS**：停发心跳 → `[HB] heartbeat LOST: node10=3 node11=3` + EMCY `0x8130`；
> 恢复 → `[HB] heartbeat OK: node10=2 node11=2` + EMCY 清除。**详情与函数级说明见 6.12**；正面解法 = 遗留 #9。

- **做什么**：把现成的 **HBconsumer（8 槽，`OD_CNT_ARR_1016=8`）真正用起来**——
  给左右两个驱动器各配一个心跳监控（索引已可用），实现「**心跳超时 → 判从站掉线 → 报 EMCY / 停机 / 恢复重连**」；
  同时用 `CO_NMT_sendCommand()` 做启动编排与故障恢复。
- **为什么值钱**：**"容错"是工业控制的硬指标**。面试常问"从站掉线/总线断你怎么处理"。
  这正好把工程里"注册了却没用"的 HBconsumer 变成亮点，也是**遗留问题 #9** 的正面解法。
- **零硬件怎么验证（✅ 已完成）**：`sim_motors.py` 已加**周期心跳帧**与 `--drop-hb-r`/`--drop-hb-l` 停发开关 →
  实测观察到心跳超时告警（`[HB] ... LOST`）+ 库自动 EMCY `0x8130` + 恢复 `[HB] ... OK`。
- **剩余**：`CO_NMT_sendCommand()` 的**自动化启动编排/故障重连状态机**尚未做成显式状态机（现只做了"超时降级 + 心跳恢复自然回 ACTIVE"）；
  真机联调时需按驱动器实际心跳周期校准 `0x1016` 超时值。

### P2 · 工程化测试（可信度背书）

- **做什么**：① 给 `agv_queue`（环形队列回绕/满/并发）与里程计正解写**单元测试**；
  ② **故障注入**（`sim_motors` 发坏帧/越界值）；③ 用 **ASan / Valgrind** 跑一遍主程序查内存与线程问题
  （调试工具见 `docs/TOOLCHAIN_DEBUG_TRIO_print.html`）。
- **为什么值钱**：把"我实现过"升级为"**我验证过、扛得住边界**"。数值/并发正确性是嵌入式岗位的加分项。
- **零硬件怎么验证**：纯本地，无需任何硬件。

> **建议顺序**：**~~P0（下行闭环）~~ → ~~P0.5（逆运动学）~~ → ~~P0 验证（vcan 闭环实跑）~~ → ~~P1（心跳容错）~~ → P2（工程化测试）**。
> P0/P0.5/P1 代码均已落地且**已通过 vcan 验证（2026-10-07 PASS）**，本项目已有**"上位机下指令 → 底盘动 → 里程计回读"的完整闭环 + 从站掉线容错**叙事；
> **下一步 = P2 工程化测试（或为真机联调做准备）**；左轮 `0x1800.eventTimer` 已顺手对齐为 `0x0100`。

---

## 十六、求职学习路线（2026-10-09 重排；**"为应聘而学"的总纲，以本节为准**）

> 本节回答"**为了应聘，接下来该学什么、学到多深**"，与第十五节的"技术增量"并行、互为素材：
> 第十五节产**项目**，本节把项目**翻译成简历与面试**。

### 16.0 定位（先钉死，所有学习照此筛）

- 目标 = **嵌入式工业控制 / AMR / AGV 的「应用与主控层」**；
  **不是** CAN 驱动开发，**不是**自研协议栈（与 §一 约束 5/6、§一「已排除方案」一致）。
- 人设一句话：**"做过 CANopen 主控的嵌入式 Linux 应用工程师"**。
- ⚠️ **已发生过的两次误判，下次勿再犯**：
  1. 把用户往"投 CAN 驱动开发岗"带；
  2. 建议深挖 **CAN 电气物理**（位定时分段 / 采样点 / TEC-REC / 位填充）——那是**写驱动 / 做协议栈**的需求，与用户定位不符。

### 16.1 岗位调研结论（2026-10-09，基于 16 条真实 JD 核实）

| 岗位类型 | CANopen 要求 | 说明 |
|---|---|---|
| AGV/AMR/工控**研发岗** | **硬项** | 深度基准 = "**精通 CANopen(PDO/SDO/NMT) + 能基于协议文档独立编写驱动程序**"；伺服方向再加 **对象字典 + DS301/DS402 + CiA402 状态机** |
| **校招线** | **了解**即可 | 表述为"了解 CANopen/EtherCAT 等工业总线" |
| AGV **实施/调试岗** | 不需要 | 与研发岗要求差一个量级 |

> **结论**：用户是**校招 + 主控/应用向**——**CANopen 应用层要深（PDO/SDO/NMT/OD/CiA402），"写驱动"不必**；
> CAN 电气物理**只记结论**（差分抗共模 / 两端 120Ω / 显性覆盖隐性）。

### 16.2 目标岗能力拆解（谁是你的命门）

| 能力 | 权重 | 现有资产 |
|---|---|---|
| **AGV/AMR 主控专项**（差速正逆解 / 里程计 / 使能时序 / 安全门限 / 心跳容错） | **命门** | 代码已全落地（§五、§十五 P0/P0.5/P1） |
| **CANopen 应用层**（PDO/SDO/NMT/对象字典/CiA402） | **命门** | 已做透 |
| Linux 系统编程（线程/锁/epoll/队列/线程池/异步日志） | 核心 | 已落地（§六） |
| 工程化（CMake / GDB / Valgrind / 测试 / 故障注入） | 加分 | 半成（§十五 P2） |
| 上板实证（i.MX6ULL 部署 + 真机 CAN 联调） | **最强背书** | 交叉编译已通，真机阻塞 |
| Qt HMI | 相对长板 | 半成品（§五） |
| C/C++ 八股（volatile/位操作/内存布局/对齐/大小端） | 门槛，必过 | 需系统过一遍 |
| STM32 + FreeRTOS + 外设 | **条件性** | 缺（Linux 侧工控岗非硬门槛） |

### 16.3 重排后的路线（按 ROI 排序，**取代旧 P1~P6**）

1. **阶段 1 · 定位与叙事骨架**（立刻，最高优先）
   一句话定位 + 项目 STAR 草稿；先搭骨架、后用证据填。
2. **阶段 2 · AGV 主控专项"讲透"**（ROI 最高）
   代码已全有，缺"能画图、能脱稿讲"：`cmd_vel → 逆解(v±ωb/2) → counts/s → 0x60FF → 使能时序 → 里程计回读 → 位姿` 一条**主控数据流大图**，加**安全门限 / 心跳降级**两个容错点。素材 = S7 / S10 / S11 分册。
3. **阶段 3 · 工程化收尾**（= 第十五节 P2 遗留）
   补故障注入 B/C/D（bad-dlc / out-of-range / flood）、跑 ASan/UBSan 主程序。把"我实现过"升级为"**我验证过**"。
4. **阶段 4 · 上板实证**（硬件到位才做，**最强背书**）
   i.MX6ULL 部署 + 真机 500K 联调（改站址 10/11、两端 120Ω、极性与 ACK 问题，见 §三 等候事项 C）。
5. **阶段 5 · C 八股 + Linux 系统编程自查**（面试前集中，贯穿）
   直接用项目里的 `agv_queue` / `agv_pool` / `agv_log` 当"我做过"证据；配 `docs/EMBEDDED_LINUX_APP_SELFCHECK_print.html`。
6. **阶段 6 · 条件性补短板（STM32 / FreeRTOS）**
   **只在你投的 JD 明确写了再补**，补到"能说清任务/队列/信号量 + 点亮一个外设"即可，不深挖。
7. **阶段 7 · 投递与反馈回补**
   面试 QA 册（`docs/AGV_CANOPEN_INTERVIEW_QA_print.html`）+ 按面试反馈回补。

### 16.4 与旧版差异 & 待办

- ✂️ **砍**：P3 里 CAN 电气 / 位定时 / 采样点的深挖（保留"四层"骨架 + CANopen 应用层即可）。
- 🔻 **降级**：STM32 + FreeRTOS 从"硬性短板"降为"**按 JD 决定**"。
- 🔺 **提升**：**AGV 主控专项**独立成最高优先级（AMR/AGV 岗命门）；**上板实证**提到核心（真机跑通 > 一堆文档）。
- **旧路线存档（2026-10-09 前，已被 16.3 取代）**：P1 定位整理 / P2 C 底层+八股 / P3 通信协议"四层结构" / P4 STM32+FreeRTOS+外设 / P5 真机上板+调试工具 / P6 STAR 叙事+投递。
- **待办**：
  1. `docs/P3_COMM_PROTOCOL_FOUR_LAYER_print.html`（通信协议"四层结构"速查，**作字典**）——**2026-10-09 生成时未落盘，需补生成**；
  2. 第十五节 P2 工程化测试收尾（故障注入 B/C/D、ASan/UBSan）；
  3. `docs/MODBUS_LEARNING_MAP_print.html`（Modbus 学习册 + 深度裁定）——**✅ 2026-10-09 已生成**；
  4. CoE 对照表（CANopen↔CoE）——待做，见 §16.5。

### 16.5 后续增量路线（2026-10-09 定：实物实验 → CoE + Modbus RTU IMU）

> 本节是 §16.3 路线在"当前阶段"的具体落点。**§16.0 的定位不变**：主控/应用向，非自研协议栈、非从站驱动开发。

**前置 · 实物实验（= §16.3 阶段 4 上板实证，最高优先）**
硬件到位后第一件事：i.MX6ULL 部署 + 真机 500K 联调（改站址 10/11、两端 120Ω、极性与 ACK 问题，见 §三 等候事项 C、第十二节末）。
**真机跑通 > 一堆文档**——它是所有后续增量的最强背书。

**增量 ① · Modbus RTU IMU（推荐：一箭双雕）**
- 做什么：IMU 走 **RS485/Modbus RTU**，主控侧写一个 **Modbus 主站**读 IMU 寄存器；数据写入**新 OD 对象**（建议 `0x7010 imu`，子1=yaw rate、子2=roll/pitch、子3=accel），再与轮式里程计 `0x6FFF` 的 θ 做**互补滤波航向融合**。
- 为什么：**同时产出两样**——「IMU 接入」与「Modbus 成果」。
- **学习深度裁定 = 应用/主站开发级（L3）**，依据见 §16.1 岗位证据（校招线只要求"了解"总线；Modbus 在 AGV 研发岗是"熟悉/优先"，只有协议栈开发岗才要"从零搭建"）：
  - **要会**：四区模型与 `0x/1x/3x/4x` 编号、功能码 `01/02/03/04/05/06/0F/10`、RTU 帧逐字节 + `CRC16(0xA001)`、`t3.5` 切帧、异常响应与异常码、Modbus TCP 的 MBAP 头、**float32 跨寄存器的 4 种字节序**、主站轮询/超时重试、RS485 半双工方向控制。
  - **不深挖**：RS485 物理层电气（收发器时序、终端电阻取值，**只记结论**）、Modbus Plus、网关/路由内核实现、自研协议栈。
- 素材：`docs/MODBUS_LEARNING_MAP_print.html`（含深度裁定、帧格式、主站状态机与本项目落地方案）。
- **软件侧已完成（2026-10-09，无硬件前提下先做掉）**：
  - 主站模块 `agv_modbus.h` / `agv_modbus.c`：CRC16(0xA001)、组帧/解帧、功能码 `0x03/0x04/0x06/0x10`、异常响应识别（`-257..-511`）、超时重试、float32 四种字序、RS485 半双工方向控制（`TIOCSRS485` 优先、RTS 兜底）、`t3.5` 帧间静默 + `tcdrain()`。
  - 单测 `tests/test_agv_modbus.c`（46 断言：CRC 向量/组帧字节序/读响应/异常/四种字序）；仿真从站 `tools/sim_imu_modbus.py`（纯标准库 pty/串口 + §10.1 寄存器表 + 异常/坏 CRC/丢帧注入）。
  - 集成：`Makefile` 与 `CMakeLists.txt` 均纳入（两处源清单 + 测试目标）；`make test` 与 `ctest` 全过；pty 端到端（C 主站 ↔ Python 从站）读回 9 个 float32 PASS，故障注入（异常码/坏 CRC/丢帧重试）均按预期。
  - 实现走读见 `docs/MODBUS_IMU_IMPL_print.html`。
- **待硬件到位后**：接真 IMU 校准 float32 字序与 RS485 方向 → 新增 OD `0x7010 imu` → 与 `0x6FFF` 做互补滤波航向融合。

**增量 ② · CoE（CANopen over EtherCAT）**
- 做什么：**知识迁移**，**不做从站协议栈开发**。产出「CANopen ↔ CoE 对照表」：`0x6040`/`0x6041`/`0x60FF` 在 CoE 的 SDO/PDO 下如何映射（复用已有 CiA-402 资产）。可选加分：SOEM 起主站 + Wireshark 抓 EtherCAT 帧。
- 为什么：对应校招 JD 的"了解 EtherCAT"；把存量 CiA-402 知识**二次变现**，几乎零新增学习成本。
- **不买硬件**；ESC 评估板 / 带 EtherCAT 的伺服等，**等投递 JD 明确要求再议**（与 §16.3 阶段 6 的"条件触发"同一原则）。
