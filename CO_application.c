/*
 * Application interface for CANopenNode.
 *
 * @file        CO_application.c
 * @author      --
 * @copyright   2021 --
 *
 * This file is part of CANopenNode, an opensource CANopen Stack.
 * Project home page is <https://github.com/CANopenNode/CANopenNode>.
 * For more information on CANopen see <http://www.can-cia.org/>.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#include "CO_application.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include "OD.h"
#include "OD_2nd.h"
#include "agv_log.h"
#include "agv_kinematics.h"   /* 里程计正解/逆解纯函数(P2 可测性重构) */

/* ===================== 日志：统一分级 + 异步落盘 =====================
 * 见 LOG_ERROR_HANDLING_print.html §2 与 AGV_ASYNC_LOG_IMPL_print.html。
 * 日志通道封装在 agv_log 模块里(RT 只 vsnprintf + 非阻塞入队，工作线程落
 * stdout)。这里只用宏：AGV_LOGD/I/W/E/F，自动带文件名+行号，并支持编译期
 * 分级过滤。RT 侧丢弃计数用 agv_log_dropped() 读取。
 */

/* 迟到量超过该值(us)记一次 "over" */
#define RT_JITTER_OVER_US     200u
/* 统计窗口：每累计这么多拍输出一行(=1000 拍 ≈ 1 秒) */
#define RT_STAT_TICKS         1000u

/* ===================== 第 7 站：里程计(差动运动学) =====================
 * 标定参数(ODOM_*)与纯函数(agv_odom_step / agv_inverse_kinematics)已抽到
 * agv_kinematics.h，便于脱离 RT/CAN 单元测试(见 P2 §1.3)。 */

/* 收帧钩子(H6064_write) → RT 拍钩子(app_programRt) 的共享数据。
 * 二者同处 RT 线程、同圈先后执行，故无需加锁(见分册 §2.3)。 */
static int32_t odom_posR_raw;       /* 右轮最近一次累计位置(计数) 0x38A */
static int32_t odom_posL_raw;       /* 左轮最近一次累计位置(计数) 0x38B */
static bool_t  odom_new;            /* true=自上拍以来收到过新位置 */

/* 里程计状态：车体位姿 */
static double  odom_x;              /* 世界坐标 x，米 */
static double  odom_y;              /* 世界坐标 y，米 */
static double  odom_th;             /* 车头朝向 θ，弧度(0=朝 +x) */

/* RPDO 收到的实际位置(0x6064)经此钩子落地。
 * stream->object 里存的是发送方 COB-ID(0x038A 右 / 0x038B 左)，
 * 用来区分这是哪一个电机的帧。只"存值 + 置旗"，积分留给
 * app_programRt 在真节拍里做(见分册 §2.3/§2.4)。 */
ODR_t H6064_write(OD_stream_t *stream, const void *buf,
                   OD_size_t count, OD_size_t *countWritten)
{
    if (stream == NULL || stream->object == NULL || buf == NULL
        || countWritten == NULL) {
        return ODR_DEV_INCOMPAT;
    }
    uint16_t *can_id = (uint16_t*)stream->object;
    int32_t   pos    = *(int32_t*)buf;      /* 已解析好的位置值(计数) */

    if (*can_id == 0x038A)      odom_posR_raw = pos;   /* 右轮 */
    else if (*can_id == 0x038B) odom_posL_raw = pos;   /* 左轮 */
    else return ODR_DEV_INCOMPAT;

    odom_new = true;            /* 告诉 RT 拍钩子：有新数据了 */
    *countWritten = count;      /* 规范：回填写入字节数 */
    return ODR_OK;
}


OD_extension_t H6064_extentionR = {
    .object = NULL,
    .read = NULL,
    .write = H6064_write,
};
OD_extension_t H6064_extentionL = {
    .object = NULL,
    .read = NULL,
    .write = H6064_write,
};

/* ===================== 第 8 站：下行控制(CiA402) =====================
 * 上行(第 7 站)是"听"：从站经 RPDO 把 0x6041 状态字、0x6064 位置回给我们。
 * 下行是"说"：我们(作 NMT 主站)经 TPDO 把 0x6040 控制字、0x60FF 目标速度
 * 发给从站。本里程碑只做"使能时序"——把两个从站推进到 Operation enabled，
 * 目标速度恒为 0(安全)，暂不下发运动指令(那属 P0.5 差速逆运动学)。
 *
 * 两本 OD 的不对称坑(见分册 S10 §3)：
 *   - OD(右轮) 的 0x6040/0x60FF 属性本就是 ODA_TPDO，0x1A00 映射也已就绪；
 *   - OD_2nd(左轮) 原属性是 ODA_RPDO(不可发)，已改成 ODA_TPDO；0x1A00
 *     默认空映射，需在 app_programStart 里运行时补齐。
 */

/* 本节点作 NMT 主站，用这两个 TPDO COB-ID 向左右从站下发(bit31=0 才有效) */
#define DL_TPDO_COBID_R   0x0000050Au   /* 右轮 TPDO: 0x50A */
#define DL_TPDO_COBID_L   0x0000050Bu   /* 左轮 TPDO: 0x50B */
#define DL_NODE_R         10u           /* 右轮从站节点号 */
#define DL_NODE_L         11u           /* 左轮从站节点号 */

/* 收帧钩子(H6041_write) → 使能推进(app_programRt) 的共享数据。
 * 同处 RT 线程、同圈先后执行，无需加锁(同第 7 站的 odom_*)。
 * 上电初始为 0，首个状态字到来前 cia402_step 会返回安全态 0x0006。 */
static uint16_t dl_swR;             /* 右轮最近一次 0x6041 状态字 */
static uint16_t dl_swL;             /* 左轮最近一次 0x6041 状态字 */

/* 状态字(0x6041)经 RPDO(映射 0x60410010) 到达后落到此钩子。
 * 范式同 H6064_write：stream->object 里存的是发送方 COB-ID(0x038A/0x038B)，
 * 用来分辨左右电机。只"存值"，推进留给 app_programRt 在真节拍里做。
 * 注意：0x6041 在两本 OD 里 dataOrig 都是 NULL(值被丢弃)，必须靠这个
 * 写钩子才能把状态字接住，否则下行使能无反馈可依。 */
ODR_t H6041_write(OD_stream_t *stream, const void *buf,
                  OD_size_t count, OD_size_t *countWritten)
{
    if (stream == NULL || stream->object == NULL || buf == NULL
        || countWritten == NULL) {
        return ODR_DEV_INCOMPAT;
    }
    uint16_t *can_id = (uint16_t*)stream->object;
    uint16_t  sw     = *(uint16_t*)buf;     /* 状态字 */

    if (*can_id == 0x038A)      dl_swR = sw;   /* 右轮 */
    else if (*can_id == 0x038B) dl_swL = sw;   /* 左轮 */
    else return ODR_DEV_INCOMPAT;

    *countWritten = count;
    return ODR_OK;
}

/* 0x6041 状态字扩展：只为接住状态字。read=NULL(禁用读——本项目不读它)。 */
OD_extension_t H6041_extentionR = {
    .object = NULL,
    .read = NULL,
    .write = H6041_write,
};
OD_extension_t H6041_extentionL = {
    .object = NULL,
    .read = NULL,
    .write = H6041_write,
};

/* 0x6040 控制字扩展：本身是可正常读写的 OD 变量，挂扩展的唯一目的是
 * "启用 flagsPDO"——OD_getFlagsPDO 要求 entry->extension != NULL，否则
 * OD_requestTPDO 无效、TPDO 不会因控制字变化而发送。用
 * OD_readOriginal/OD_writeOriginal 保留原始读写行为。 */
OD_extension_t H6040_extentionR = {
    .object = NULL,
    .read = OD_readOriginal,
    .write = OD_writeOriginal,
};
OD_extension_t H6040_extentionL = {
    .object = NULL,
    .read = OD_readOriginal,
    .write = OD_writeOriginal,
};

/* CiA402 使能时序(纯函数：由状态字推下一步控制字)。掩码对照见分册表 10-1：
 *   Switch_on_disabled  (sw&0x4F)==0x40 → 0x0006 Shutdown
 *   Ready_to_switch_on  (sw&0x6F)==0x21 → 0x0007 Switch on
 *   Switched_on         (sw&0x6F)==0x23 → 0x000F Enable operation
 *   Operation_enabled   (sw&0x6F)==0x27 → 0x000F 保持使能
 *   Fault               (sw&0x4F)==0x08 → 0x0080 Fault reset
 * 其它/未知 → 0x0006(退回安全态)。目标速度恒为 0，故使能后即静止。 */
static uint16_t cia402_step(uint16_t sw)
{
    if ((sw & 0x004Fu) == 0x0008u) return 0x0080u;  /* Fault → Fault reset */
    if ((sw & 0x004Fu) == 0x0040u) return 0x0006u;  /* Switch on disabled → Shutdown */
    if ((sw & 0x006Fu) == 0x0021u) return 0x0007u;  /* Ready to switch on → Switch on */
    if ((sw & 0x006Fu) == 0x0023u) return 0x000Fu;  /* Switched on → Enable operation */
    if ((sw & 0x006Fu) == 0x0027u) return 0x000Fu;  /* Operation enabled → 保持 */
    return 0x0006u;                                 /* 其它 → Shutdown(安全) */
}

/* ===================== 第 11 站：差速逆运动学 + 运动下发 =====================
 * 上位机经 SDO 写 0x7000[1]=v(m/s), [2]=ω(rad/s)(cmd_vel)。RT 拍把它逆解成
 * 两轮轮面线速度 vR=v+ω·b/2、vL=v−ω·b/2(见分册 S11 §1)，换算成 counts/s，
 * 过安全门限后写入目标速度 0x60FF(第 10 站已把 0x60FF 映射进 TPDO)。
 *
 * 两个安全门限(见分册 S11 §5.4)：
 *   ① 未使能不动：只有两轮都在 Operation enabled 才允许非零轮速；
 *   ② 失联停车(deadman)：超时未收到新 cmd_vel 就把目标速度清零。
 *      deadman 不能只靠"数值是否变化"判断——主机持续发同一恒速时数值不变，
 *      必须由"写动作"本身打时间戳，故给 0x7000 挂写钩子(H7000_write)。
 */

/* cmd_vel 失联判据：RT 真节拍为 1ms/拍，500 拍 = 500ms 无新指令即停车 */
#define CMD_TIMEOUT_TICKS  500u

static uint32_t rt_tick_now;        /* RT 真节拍单调计数(1ms/拍) */
static uint32_t cmd_rx_tick;        /* 最近一次收到 0x7000 写入时刻的节拍号 */

/* 0x7000 写钩子：SDO 写 cmd_vel 时被调用(SDO 服务端已在外层持 CO_LOCK_OD)。
 * 先落值(OD_writeOriginal)，再刷新 deadman 时间戳。 */
ODR_t H7000_write(OD_stream_t *stream, const void *buf,
                  OD_size_t count, OD_size_t *countWritten)
{
    ODR_t ret = OD_writeOriginal(stream, buf, count, countWritten);
    if (ret == ODR_OK) cmd_rx_tick = rt_tick_now;
    return ret;
}

OD_extension_t H7000_extention = {
    .object = NULL,
    .read = OD_readOriginal,
    .write = H7000_write,
};

/* 0x60FF 目标速度扩展：本身是普通 OD 变量，挂扩展的唯一目的是"启用
 * flagsPDO"——OD_getFlagsPDO 要求 entry->extension != NULL，否则
 * OD_requestTPDO 无效、TPDO 不会因目标速度变化而发送(同 0x6040 的成法)。 */
OD_extension_t H60FF_extentionR = {
    .object = NULL,
    .read = OD_readOriginal,
    .write = OD_writeOriginal,
};
OD_extension_t H60FF_extentionL = {
    .object = NULL,
    .read = OD_readOriginal,
    .write = OD_writeOriginal,
};

/******************************************************************************/
CO_ReturnError_t app_programStart(uint16_t *bitRate,
                                  uint8_t *nodeId,
                                  uint32_t *errInfo)
{
    CO_ReturnError_t err = CO_ERROR_NO;

    //*************************************
    // set sync
    //*************************************
    ODR_t odRet;
    // Set SYNC Period
    odRet = OD_set_u32(OD_ENTRY_H1006_communicationCyclePeriod, 0, 100000, true);
    if (odRet != ODR_OK) {
        if (errInfo != NULL) *errInfo = OD_getIndex(OD_ENTRY_H1006_communicationCyclePeriod);
        return CO_ERROR_OD_PARAMETERS;
    }
    // Enable SYNC
    odRet = OD_set_u32(OD_ENTRY_H1005_COB_ID_SYNCMessage, 0, 0x40000080, true);
    if (odRet != ODR_OK) {
        if (errInfo != NULL) *errInfo = OD_getIndex(OD_ENTRY_H1005_COB_ID_SYNCMessage);
        return CO_ERROR_OD_PARAMETERS;
    }

    //*************************************
    // Set RPDO for Right Motor
    //*************************************
    odRet = OD_set_u32(OD_ENTRY_H1400_RPDOCommunicationParameter, 1, 0x0000038A, true);
    if (odRet != ODR_OK) {
        if (errInfo != NULL) *errInfo = OD_getIndex(OD_ENTRY_H1400_RPDOCommunicationParameter);
        return CO_ERROR_OD_PARAMETERS;
    }
    odRet = OD_set_u8(OD_ENTRY_H1600_RPDOMappingParameter, 0, 2, true);
    if (odRet != ODR_OK) {
        if (errInfo != NULL) *errInfo = OD_getIndex(OD_ENTRY_H1600_RPDOMappingParameter);
        return CO_ERROR_OD_PARAMETERS;
    }
    odRet = OD_set_u32(OD_ENTRY_H1600_RPDOMappingParameter, 1, 0x60410010, true);
    if (odRet != ODR_OK) {
        if (errInfo != NULL) *errInfo = OD_getIndex(OD_ENTRY_H1600_RPDOMappingParameter);
        return CO_ERROR_OD_PARAMETERS;
    }
    odRet = OD_set_u32(OD_ENTRY_H1600_RPDOMappingParameter, 2, 0x60640020, true);
    if (odRet != ODR_OK) {
        if (errInfo != NULL) *errInfo = OD_getIndex(OD_ENTRY_H1600_RPDOMappingParameter);
        return CO_ERROR_OD_PARAMETERS;
    }

    /* malloc 必须判 NULL，否则 *objR 直接段错误 */
    uint16_t *objR = (uint16_t*)malloc(sizeof(uint16_t));
    if (objR == NULL) {
        AGV_LOGF("malloc objR(RPDO 0x038A) 失败: %s", strerror(errno));
        return CO_ERROR_OUT_OF_MEMORY;
    }
    *objR = 0x038A;
    H6064_extentionR.object = objR;
    OD_extension_init(OD_ENTRY_H6064_positionActualValue, &H6064_extentionR);


    //*************************************
    // Set RPDO for Left Motor
    //*************************************
    odRet = OD_set_u32(OD_2nd_ENTRY_H1401_RPDOCommunicationParameter, 1, 0x0000038B, true);
    if (odRet != ODR_OK) {
        if (errInfo != NULL) *errInfo = OD_getIndex(OD_2nd_ENTRY_H1401_RPDOCommunicationParameter);
        return CO_ERROR_OD_PARAMETERS;
    }
    odRet = OD_set_u8(OD_2nd_ENTRY_H1601_RPDOMappingParameter, 0, 2, true);
    if (odRet != ODR_OK) {
        if (errInfo != NULL) *errInfo = OD_getIndex(OD_2nd_ENTRY_H1601_RPDOMappingParameter);
        return CO_ERROR_OD_PARAMETERS;
    }
    odRet = OD_set_u32(OD_2nd_ENTRY_H1601_RPDOMappingParameter, 1, 0x60410010, true);
    if (odRet != ODR_OK) {
        if (errInfo != NULL) *errInfo = OD_getIndex(OD_2nd_ENTRY_H1601_RPDOMappingParameter);
        return CO_ERROR_OD_PARAMETERS;
    }
    odRet = OD_set_u32(OD_2nd_ENTRY_H1601_RPDOMappingParameter, 2, 0x60640020, true);
    if (odRet != ODR_OK) {
        if (errInfo != NULL) *errInfo = OD_getIndex(OD_2nd_ENTRY_H1601_RPDOMappingParameter);
        return CO_ERROR_OD_PARAMETERS;
    }

    uint16_t *objL = (uint16_t*)malloc(sizeof(uint16_t));
    if (objL == NULL) {
        AGV_LOGF("malloc objL(RPDO 0x038B) 失败: %s", strerror(errno));
        return CO_ERROR_OUT_OF_MEMORY;
    }
    *objL = 0x038B;
    H6064_extentionL.object = objL;
    OD_extension_init(OD_2nd_ENTRY_H6064_positionActualValue, &H6064_extentionL);

    //*************************************
    // TPDO(下行)：右/左轮 控制字 0x6040 + 目标速度 0x60FF
    // 关键：本函数在 CO_CANopenInit 之前调用，此处写入的映射/COB-ID
    // 会被随后的 CO_TPDO_init 读到并生效。
    //*************************************
    /* 右轮 TPDO：清 bit31 使 0xC000050A → 0x0000050A(bit31=1 表示 PDO 无效)。
     * 映射(0x1A00 = 0x6040+0x60FF)在 OD.c 里已就绪，无需再改。 */
    odRet = OD_set_u32(OD_ENTRY_H1800_TPDOCommunicationParameter, 1, DL_TPDO_COBID_R, true);
    if (odRet != ODR_OK) {
        if (errInfo != NULL) *errInfo = OD_getIndex(OD_ENTRY_H1800_TPDOCommunicationParameter);
        return CO_ERROR_OD_PARAMETERS;
    }

    /* 左轮 TPDO：OD_2nd 的 0x1A00 默认空映射，需补齐 sub0..2；再清 bit31
     * 使 0xC0000180 → 0x0000050B。
     * 只改 sub1(COB-ID)——OD_2nd 的 0x1800 子索引布局(6 个)与 OD.c(5 个)
     * 不同，不要盲改其它子索引以免错位。 */
    odRet = OD_set_u8(OD_2nd_ENTRY_H1A00_TPDOMappingParameter, 0, 2, true);
    if (odRet != ODR_OK) {
        if (errInfo != NULL) *errInfo = OD_getIndex(OD_2nd_ENTRY_H1A00_TPDOMappingParameter);
        return CO_ERROR_OD_PARAMETERS;
    }
    odRet = OD_set_u32(OD_2nd_ENTRY_H1A00_TPDOMappingParameter, 1, 0x60400010, true);
    if (odRet != ODR_OK) {
        if (errInfo != NULL) *errInfo = OD_getIndex(OD_2nd_ENTRY_H1A00_TPDOMappingParameter);
        return CO_ERROR_OD_PARAMETERS;
    }
    odRet = OD_set_u32(OD_2nd_ENTRY_H1A00_TPDOMappingParameter, 2, 0x60FF0020, true);
    if (odRet != ODR_OK) {
        if (errInfo != NULL) *errInfo = OD_getIndex(OD_2nd_ENTRY_H1A00_TPDOMappingParameter);
        return CO_ERROR_OD_PARAMETERS;
    }
    odRet = OD_set_u32(OD_2nd_ENTRY_H1800_TPDOCommunicationParameter, 1, DL_TPDO_COBID_L, true);
    if (odRet != ODR_OK) {
        if (errInfo != NULL) *errInfo = OD_getIndex(OD_2nd_ENTRY_H1800_TPDOCommunicationParameter);
        return CO_ERROR_OD_PARAMETERS;
    }

    /* 挂扩展：0x6040 启用 flagsPDO(控制字变化即触发 TPDO)；
     * 0x6041 接住状态字。.object 复用 objR/objL(存 COB-ID 0x038A/0x038B)，
     * 供写钩子分辨左右。 */
    H6040_extentionR.object = objR;
    OD_extension_init(OD_ENTRY_H6040_controlword, &H6040_extentionR);
    H6041_extentionR.object = objR;
    OD_extension_init(OD_ENTRY_H6041_statusword, &H6041_extentionR);

    H6040_extentionL.object = objL;
    OD_extension_init(OD_2nd_ENTRY_H6040_controlword, &H6040_extentionL);
    H6041_extentionL.object = objL;
    OD_extension_init(OD_2nd_ENTRY_H6041_statusword, &H6041_extentionL);

    /* 第 11 站：0x7000 cmd_vel 写钩子(落值 + 刷 deadman 时间戳)；
     * 0x60FF 目标速度挂扩展，启用 flagsPDO，使 RT 写后 OD_requestTPDO 能触发 TPDO。 */
    OD_extension_init(OD_ENTRY_H7000_agvCmdVel, &H7000_extention);
    OD_extension_init(OD_ENTRY_H60FF_targetVelocity, &H60FF_extentionR);
    OD_extension_init(OD_2nd_ENTRY_H60FF_targetVelocity, &H60FF_extentionL);

    return err;
}


/******************************************************************************/
void app_communicationReset(CO_t *co) {

    /* example printouts */
    if (!co->nodeIdUnconfigured) {
        /* CANopen Node-ID is configured and all services will work. */
    }
    else {
        /* CANopen Node-ID is unconfigured, so only LSS slave will work. */
    }
}


/******************************************************************************/
void app_programEnd() {
    /* 日志生命周期(agv_log_init/agv_log_deinit)现由 main 统一管理：
     * main 结尾还有收尾日志，此处若关日志会把它丢掉，故不再关闭。 */
}


/******************************************************************************/
void app_programAsync(CO_t *co, uint32_t timer1usDiff) {
    (void) timer1usDiff; /* unused */

    /* ===================== 第 8 站：NMT 主站"叫醒"从站 =====================
     * 本节点作 NMT 主站，上电后把左右两个从站切到 OPERATIONAL——从站只有
     * 进入 OP 才会处理/回送 PDO。用静态标志保证只发一次。 */
    static bool_t nmt_sent = false;
    if (!nmt_sent) {
        nmt_sent = true;
        CO_NMT_sendCommand(co->NMT, CO_NMT_ENTER_OPERATIONAL, DL_NODE_R);
        CO_NMT_sendCommand(co->NMT, CO_NMT_ENTER_OPERATIONAL, DL_NODE_L);
        AGV_LOGI("[DL] NMT start remote node: R=%u L=%u", DL_NODE_R, DL_NODE_L);
    }
}


/******************************************************************************/
void app_programRt(CO_t *co, uint32_t timerLate_us, bool_t timerEvent) {
    /* ===================== 定时唤醒抖动统计 =====================
     * rt_thread 每圈调用本函数。epoll_wait 的返回原因有三类：
     *   ① timer_fd 定时器到点 → timerEvent=true，才代表一个"定时节拍(tick)"
     *   ② event_fd 跨线程门铃 → 主线程有 CAN 报文要发时提前叫醒 RT 线程
     *   ③ CAN 报文到达 / EINTR 信号
     * ②③ 不是节拍，只计数、不参与抖动统计。
     *
     * 度量口径：迟到量(lateness) = 本次定时器唤醒时刻 - 本拍在理想栅格上的时刻。
     * timerfd 是相位锁定的绝对时基(rt 模式不 re-arm)，理想栅格为
     *   timerStart_us + k * 1000us
     * 其中 timerStart_us 是 CO_epoll_create 里 timerfd_settime 的时刻。
     * 迟到量恒落在 [0,1000)us，直接由 CO_epoll_wait 的 timer 分支算出
     * (ep->timerLate_us) 后传进来，本函数只做窗口统计与打印。
     */
    static uint32_t win_n;          /* 本窗口节拍数 */
    static uint32_t win_lat_sum;    /* 本窗口迟到量之和 */
    static uint32_t win_lat_min = UINT32_MAX;
    static uint32_t win_lat_max;    /* 本窗口最大迟到量 */
    static uint32_t win_over;       /* 迟到量 > RT_JITTER_OVER_US 的节拍数 */
    static uint32_t win_other;      /* 本窗口非定时唤醒次数(门铃/CAN接收/信号) */
    static uint32_t total;          /* 累计节拍数 */

    if (!timerEvent) {
        win_other++;                /* 门铃/CAN接收/信号唤醒，不是节拍 */
        return;
    }

    uint32_t lateness = timerLate_us;

    win_n++;
    win_lat_sum += lateness;
    if (lateness < win_lat_min) win_lat_min = lateness;
    if (lateness > win_lat_max) win_lat_max = lateness;
    if (lateness > RT_JITTER_OVER_US) win_over++;
    total++;

    if (win_n >= RT_STAT_TICKS) {
        AGV_LOGI("[RT] tick n=%u lat_avg=%u lat_min=%u lat_max=%u over%u=%u other=%u total=%u drop=%llu",
                 win_n, win_lat_sum / win_n, win_lat_min, win_lat_max,
                 RT_JITTER_OVER_US, win_over, win_other, total,
                 (unsigned long long)agv_log_dropped());

        win_n = 0; win_lat_sum = 0; win_lat_min = UINT32_MAX;
        win_lat_max = 0; win_over = 0; win_other = 0;
    }

    /* ===================== 第 8 站：下行控制(CiA402 使能推进) =====================
     * 每个真节拍依据最近状态字推进一步控制字，仅当变化时写 OD 并触发 TPDO。
     * 目标速度 0x60FF 恒为默认值 0(安全)，本里程碑只做"使能"，不发运动指令。
     * 左轮上电尚未收到状态字时 dl_swL=0 → cia402_step 返回 0x0006(安全态)。
     * 写侧在 RT 线程、网关读侧在 main 线程，跨线程访问 OD_RAM，用 CO_LOCK_OD 保护。 */
    uint16_t cwR = cia402_step(dl_swR);
    uint16_t cwL = cia402_step(dl_swL);

    CO_LOCK_OD(co->CANmodule);
    if (cwR != OD_RAM.x6040_controlword) {
        OD_RAM.x6040_controlword = cwR;
        OD_requestTPDO(OD_getFlagsPDO(OD_ENTRY_H6040_controlword), 0);
    }
    if (cwL != OD_2nd_RAM.x6040_controlword) {
        OD_2nd_RAM.x6040_controlword = cwL;
        OD_requestTPDO(OD_getFlagsPDO(OD_2nd_ENTRY_H6040_controlword), 0);
    }
    CO_UNLOCK_OD(co->CANmodule);

    /* ===================== 第 11 站：逆解 + 目标速度下发 =====================
     * 顺序：读 cmd_vel(锁内取快照) → 逆解 → 换算 → 安全门限 → 写 0x60FF。
     * 前提：位置在真节拍段(上面已把非节拍 return 掉)。 */
    rt_tick_now++;

    float32_t cmd_v, cmd_w;
    CO_LOCK_OD(co->CANmodule);
    cmd_v = OD_RAM.x7000_agvCmdVel[0];      /* 子索引 1：v (m/s) */
    cmd_w = OD_RAM.x7000_agvCmdVel[1];      /* 子索引 2：ω (rad/s) */
    uint32_t cmd_age = rt_tick_now - cmd_rx_tick;   /* 距上次下发经过的拍数 */
    CO_UNLOCK_OD(co->CANmodule);

    /* ① 逆解：车体 (v,ω) → 两轮目标速度 (counts/s)。纯函数，可单测。 */
    int32_t vR_cnt, vL_cnt;
    agv_inverse_kinematics((double)cmd_v, (double)cmd_w, &vR_cnt, &vL_cnt);

    /* ③ 安全门限：未使能 或 cmd_vel 失联 或 从站心跳超时 → 强制零(见分册 S11 §5.4) */
    bool_t cmd_timeout = (cmd_age > CMD_TIMEOUT_TICKS);

    /* ④ 心跳容错：任一所监控从站心跳超时 → 判掉线 → 强制零速。
     * 监控槽在 OD.c 配置：0x1016[1]=node10(右)、[2]=node11(左)，getState 的
     * idx 为 0 基数组下标(槽 1→idx0、槽 2→idx1)。EMCY 由 HBconsumer 库内自动
     * 上报，这里只做运动降级；仅在状态跃迁到/退出 TIMEOUT 时打一行日志。 */
    CO_HBconsumer_state_t hbR = CO_HBconsumer_getState(co->HBcons, 0);
    CO_HBconsumer_state_t hbL = CO_HBconsumer_getState(co->HBcons, 1);
    bool_t hb_lost = (hbR == CO_HBconsumer_TIMEOUT || hbL == CO_HBconsumer_TIMEOUT);
    static bool_t hb_lost_prev;
    if (hb_lost != hb_lost_prev) {
        AGV_LOGE("[HB] heartbeat %s: node%u=%u node%u=%u",
                 hb_lost ? "LOST" : "OK", DL_NODE_R, (unsigned)hbR,
                 DL_NODE_L, (unsigned)hbL);
        hb_lost_prev = hb_lost;
    }

    if ((dl_swR & 0x006Fu) != 0x0027u || (dl_swL & 0x006Fu) != 0x0027u
        || cmd_timeout || hb_lost) {
        vR_cnt = 0;
        vL_cnt = 0;
    }

    /* ④ 写入并触发 TPDO：仅当变化时发，减少总线占用(0x60FF 已挂扩展) */
    CO_LOCK_OD(co->CANmodule);
    if (vR_cnt != OD_RAM.x60FF_targetVelocity) {
        OD_RAM.x60FF_targetVelocity = vR_cnt;
        OD_requestTPDO(OD_getFlagsPDO(OD_ENTRY_H60FF_targetVelocity), 0);
    }
    if (vL_cnt != OD_2nd_RAM.x60FF_targetVelocity) {
        OD_2nd_RAM.x60FF_targetVelocity = vL_cnt;
        OD_requestTPDO(OD_getFlagsPDO(OD_2nd_ENTRY_H60FF_targetVelocity), 0);
    }
    CO_UNLOCK_OD(co->CANmodule);

    /* ===================== 第 7 站：里程计积分 =====================
     * 只在真节拍(上面已把非节拍 return 掉)且本拍收到过新位置(odom_new)
     * 时积分一次，避免被门铃/报文唤醒或 1ms 内多帧重复积分。
     * 存(H6064_write)与算(此处)同处 RT 线程、同圈先后执行，故共享变量
     * 无需加锁(见分册 §2.3)。
     */
    static int32_t prevR, prevL;    /* 上拍累计位置(计数) */
    static bool_t  odom_init;       /* 首拍只记起点、不积分 */

    if (!odom_new) return;
    odom_new = false;

    if (!odom_init) {               /* 首次：把起点记为 0 */
        prevR = odom_posR_raw;
        prevL = odom_posL_raw;
        odom_init = true;
        odom_x = odom_y = odom_th = 0.0;
        return;
    }

    /* ①②③ 积分交给纯函数 agv_odom_step(可脱离 RT/CAN 单测，见 P2 §1.7) */
    agv_odom_step(prevR, prevL, odom_posR_raw, odom_posL_raw,
                  &odom_x, &odom_y, &odom_th);
    prevR = odom_posR_raw;
    prevL = odom_posL_raw;

    /* 把位姿写进自定义 OD 数组 0x6FFF(索引 1/2/3 = x/y/th)，供 CiA-309
     * 网关(Qt 上位机)按 SDO 读取。写侧在 rt_thread，网关读侧在 main 线程，
     * 跨线程访问同一 OD_RAM，必须用 CO_LOCK_OD 保护。 */
    CO_LOCK_OD(co->CANmodule);
    OD_RAM.x6FFF_agvOdometry[0] = (float32_t)odom_x;
    OD_RAM.x6FFF_agvOdometry[1] = (float32_t)odom_y;
    OD_RAM.x6FFF_agvOdometry[2] = (float32_t)odom_th;
    CO_UNLOCK_OD(co->CANmodule);

    /* 每 1000 拍(≈1s)记录一次，避免 RT 线程终端 IO 拖垮实时性 */
    static uint32_t odom_n;
    if (++odom_n >= 1000u) {
        odom_n = 0;
        AGV_LOGI("[ODOM] x=%.3f y=%.3f th=%.1fdeg",
                 odom_x, odom_y, odom_th * 180.0 / ODOM_PI);
    }
#if 0
    /* Simulation: detect change of state of the variable and trigger TPDO, to
     * which variable is possibly mapped. In our example x2110_variableInt32[0]
     * variable will be mapped to TPDO. If program detects change-of-state of
     * its value, TPDO will be triggered for sending. (x2110_variableInt32[0]
     * variable can be changed by SDO.)
     */

    /* static variable is like global: initialized to 0, then keeps its value */
    static int32_t value_old = 0;
    int32_t value_current = 0; //OD_RAM.x2110_variableInt32[0];

    /* Detect change of state and trigger TPDO. Of course, variable must be
     * mapped to event driven TPDO for this to have effect. */
    if (value_current != value_old) {
        OD_requestTPDO(OD_variableInt32_flagsPDO, 1); /* subindex is 1 */
    }
    value_old = value_current;
#endif
}
