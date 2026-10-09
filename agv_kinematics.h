/*
 * 差速底盘运动学：里程计正解 + 逆解（纯函数，可脱离 CAN/RT 线程单测）。
 *
 * 设计动机（见 docs/P2_ENGINEERING_TEST_KNOWLEDGE_print.html §1.3）：
 *   原积分公式直接读写 CO_application.c 里的静态位姿变量、且埋在 1ms RT 拍里，
 *   无法单测。这里把"算"从"存"里抽出来——相同输入必得相同输出、不碰任何
 *   外部状态，于是 test_agv_odom 只链接本文件即可验证中间点法与 32 位回绕。
 */
#ifndef AGV_KINEMATICS_H
#define AGV_KINEMATICS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ---- 标定参数：按实车改(见分册 S7 §3 / S11 §5.4 警告) ---- */
#define ODOM_WHEEL_R_M      0.050    /* 驱动轮半径 r，米 */
#define ODOM_COUNT_PER_REV  10000    /* 每转计数 N，counts/rev */
#define ODOM_WHEEL_BASE_M   0.300    /* 轮距 b(左右轮中心距)，米 */
#define ODOM_DIR_R          (+1)     /* 右轮方向：+1 前进为正，否则 -1 */
#define ODOM_DIR_L          (+1)     /* 左轮方向：+1 前进为正，否则 -1 */
#define ODOM_PI             3.14159265358979
/* 每 1 计数对应的轮面位移 k = 2πr / N */
#define ODOM_K_PER_COUNT    (2.0 * ODOM_PI * ODOM_WHEEL_R_M / ODOM_COUNT_PER_REV)

/*
 * 单拍里程计正解(中点法)：由上一拍/本拍两轮累计计数增量更新车体位姿 x,y,θ。
 * 计数差用 uint32 相减，天然处理 32 位回绕（计数器从 INT32_MAX 跳到
 * INT32_MIN 时仍得到一个绝对值很小的增量）。
 * 注意：调用方负责"首拍只记起点、不积分"（本函数无状态，不知是否首拍）。
 *
 * @param prevR,prevL  上一拍两轮累计计数
 * @param curR,curL    本拍两轮累计计数
 * @param x,y          世界坐标(米)，原地增量更新
 * @param th           车头朝向(弧度)，原地增量更新
 */
void agv_odom_step(int32_t prevR, int32_t prevL,
                   int32_t curR,  int32_t curL,
                   double *x, double *y, double *th);

/*
 * 差速逆解：车体目标 (v, ω) → 左右轮目标速度(counts/s)。
 *   vR_ms = v + ω·b/2 ，vL_ms = v - ω·b/2 ，再按 k 换算、乘方向系数取整。
 * @param v         期望线速度(m/s)
 * @param w         期望角速度(rad/s)
 * @param vR_cnt,vL_cnt  输出：右/左轮目标速度(counts/s)
 */
void agv_inverse_kinematics(double v, double w,
                            int32_t *vR_cnt, int32_t *vL_cnt);

#ifdef __cplusplus
}
#endif

#endif /* AGV_KINEMATICS_H */
