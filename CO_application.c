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

/* ===================== 第 7 站：里程计(差动运动学) ===================== */
/* ---- 标定参数：按实车改(见分册 §3) ---- */
#define ODOM_WHEEL_R_M      0.050    /* 驱动轮半径 r，米 */
#define ODOM_COUNT_PER_REV  10000    /* 每转计数 N，counts/rev */
#define ODOM_WHEEL_BASE_M   0.300    /* 轮距 b(左右轮中心距)，米 */
#define ODOM_DIR_R          (+1)     /* 右轮方向：+1 前进为正，否则 -1 */
#define ODOM_DIR_L          (+1)     /* 左轮方向：+1 前进为正，否则 -1 */
#define ODOM_PI             3.14159265358979
/* 每 1 计数对应的轮面位移 k = 2πr / N */
#define ODOM_K_PER_COUNT    (2.0 * ODOM_PI * ODOM_WHEEL_R_M / ODOM_COUNT_PER_REV)

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
    (void) co; (void) timer1usDiff; /* unused */
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

    /* ① 本拍两轮各转了多少计数(无符号相减，天然处理 32 位回绕) */
    int32_t dCntR = (int32_t)((uint32_t)odom_posR_raw - (uint32_t)prevR);
    int32_t dCntL = (int32_t)((uint32_t)odom_posL_raw - (uint32_t)prevL);
    prevR = odom_posR_raw;
    prevL = odom_posL_raw;

    /* ① 计数 → 轮面位移(m)，并用方向系数统一「前进为正」 */
    double dR = ODOM_DIR_R * ODOM_K_PER_COUNT * (double)dCntR;
    double dL = ODOM_DIR_L * ODOM_K_PER_COUNT * (double)dCntL;

    /* ② 两轮 → 车体位移/转角 */
    double d   = (dR + dL) / 2.0;
    double dth = (dR - dL) / ODOM_WHEEL_BASE_M;

    /* ③ 中点法累加位姿(θmid 用更新前的 θ，θ 最后才加) */
    double thmid = odom_th + dth / 2.0;
    odom_x  += d * cos(thmid);
    odom_y  += d * sin(thmid);
    odom_th += dth;

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
