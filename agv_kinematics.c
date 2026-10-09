/*
 * 差速底盘运动学实现（见 agv_kinematics.h）。
 */

#include "agv_kinematics.h"

#include <stddef.h>   /* NULL */
#include <math.h>     /* cos / sin */

void agv_odom_step(int32_t prevR, int32_t prevL,
                   int32_t curR,  int32_t curL,
                   double *x, double *y, double *th)
{
    if (x == NULL || y == NULL || th == NULL) {
        return;
    }

    /* ① 本拍两轮各转了多少计数(无符号相减，天然处理 32 位回绕) */
    int32_t dCntR = (int32_t)((uint32_t)curR - (uint32_t)prevR);
    int32_t dCntL = (int32_t)((uint32_t)curL - (uint32_t)prevL);

    /* ② 计数 → 轮面位移(m)，并用方向系数统一「前进为正」 */
    double dR = ODOM_DIR_R * ODOM_K_PER_COUNT * (double)dCntR;
    double dL = ODOM_DIR_L * ODOM_K_PER_COUNT * (double)dCntL;

    /* ③ 两轮 → 车体位移/转角 */
    double d   = (dR + dL) / 2.0;
    double dth = (dR - dL) / ODOM_WHEEL_BASE_M;

    /* ④ 中点法累加位姿(θmid 用更新前的 θ，θ 最后才加) */
    double thmid = *th + dth / 2.0;
    *x  += d * cos(thmid);
    *y  += d * sin(thmid);
    *th += dth;
}

void agv_inverse_kinematics(double v, double w,
                            int32_t *vR_cnt, int32_t *vL_cnt)
{
    /* ① 逆解：车体 (v,ω) → 两轮轮面线速度 (m/s) */
    double vR_ms = v + w * ODOM_WHEEL_BASE_M / 2.0;
    double vL_ms = v - w * ODOM_WHEEL_BASE_M / 2.0;

    /* ② 单位换算：轮面线速度 → 目标速度 (counts/s)，用 ODOM_DIR_* 统一正方向 */
    if (vR_cnt != NULL) {
        *vR_cnt = (int32_t)(ODOM_DIR_R * vR_ms / ODOM_K_PER_COUNT);
    }
    if (vL_cnt != NULL) {
        *vL_cnt = (int32_t)(ODOM_DIR_L * vL_ms / ODOM_K_PER_COUNT);
    }
}
