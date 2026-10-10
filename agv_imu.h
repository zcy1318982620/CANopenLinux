/*
 * IMU 采集模块（Modbus RTU 主站 + 后台轮询线程）。
 *
 * 职责边界（与 agv_modbus 一样"只做一层"）：
 *   - 用 agv_modbus 周期读 IMU 的 0x04 输入寄存器（18 个 = 9×float32）；
 *   - 把最新一帧发布成"快照"，供 RT 拍无锁读取（seqlock）；
 *   - 维护成功/失败计数与从站离线判定。
 * 本模块**不依赖 agv_log、不依赖 OD/CANopen**：错误以返回码/计数器表达，
 * 由调用方(CO_application.c)分级打印；这样 test_agv_imu 只链接
 * agv_imu.c + agv_modbus.c 就能测纯函数 decode。
 *
 * 线程模型：agv_imu_start() 起一条普通(非 RT)线程循环轮询；agv_imu_stop()
 * 置位停止标志后 join。一个模块实例对应一条串口。
 */

#ifndef AGV_IMU_H
#define AGV_IMU_H

#include <stdint.h>

#include "agv_modbus.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 轮询周期(ms)：≈20Hz。对航向融合足够，且不会把 RS485 占满。 */
#ifndef AGV_IMU_POLL_MS
#define AGV_IMU_POLL_MS 50u
#endif

/* IMU 输入寄存器区基地址与寄存器数（学习册 §10.1 / sim_imu_modbus.py）：
 * 0x0000~0x0011 = 9×float32 = 姿态(3) + 角速度(3) + 加速度(3)。 */
#define AGV_IMU_REG_BASE   0x0000u
#define AGV_IMU_REG_COUNT  18u

/* 一帧 IMU 采样，字段顺序与 0x04 区寄存器一一对应 */
typedef struct {
    float roll, pitch, yaw;   /* 姿态角 (rad) */
    float gx, gy, gz;         /* 角速度 (rad/s) */
    float ax, ay, az;         /* 加速度 (m/s^2) */
} agv_imu_sample_t;

/*
 * 启动采集：打开串口并创建轮询线程。
 * @param dev 串口设备路径(如 /dev/ttyS1 或无硬件时的 /dev/pts/N)
 * @param cfg 主站配置；NULL 用默认(115200/从站1/ABCD/300ms/重试2)
 * @return 0 成功；MB_E_PARAM 参数非法或已在运行；MB_E_IO 打开/建线程失败
 */
int  agv_imu_start(const char *dev, const mb_cfg_t *cfg);

/* 停止采集：置停止标志 → join 轮询线程 → 关闭串口。未启动时为无操作。 */
void agv_imu_stop(void);

/*
 * 取最新一帧快照(seqlock 无锁读，RT 拍可调用)。
 * @param out 非空
 * @return 1=有有效快照并已回填；0=尚无数据(out 不变)
 */
int  agv_imu_get(agv_imu_sample_t *out);

/* 从站是否离线(连续失败 >= MB_OFFLINE_THRESHOLD)。 */
int  agv_imu_offline(void);

/* 成功/失败事务累计计数(观测用)。 */
uint64_t agv_imu_stat_ok(void);
uint64_t agv_imu_stat_err(void);

/*
 * 纯函数(可单测)：把 18 个寄存器按字序还原成 9 个 float32。
 * regs[2k]/regs[2k+1] 为第 k 个量的低/高地址寄存器。
 */
void agv_imu_decode(const uint16_t *regs, mb_word_order_t order,
                    agv_imu_sample_t *out);

#ifdef __cplusplus
}
#endif

#endif /* AGV_IMU_H */
