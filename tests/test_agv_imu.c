/*
 * agv_imu 单元测试（零框架，见 tests/agv_test.h）。
 * 只链接 agv_imu.c + agv_modbus.c，不碰真实 IMU 硬件、不碰 CAN/RT：
 *   - agv_imu_decode：18 寄存器 → 9 个 float32，四种字序都要对；
 *   - 模块状态机：未启动时 agv_imu_get 返回 0；参数非法/设备打不开的返回码；
 *     agv_imu_stop 幂等。
 * 运行：make test 或 ctest（对应目标 test_agv_imu）。
 */

#include "agv_test.h"
#include "agv_imu.h"

#include <stdint.h>
#include <string.h>

/* 把 9 个 float 按给定字序打包成 18 个寄存器（decode 的逆操作，用于构造测试向量）。
 * 字序定义见 agv_modbus.h：ABCD 为内存字节序 A B C D 直接落到 r0/r1。 */
static void pack_regs(const float *v, mb_word_order_t order, uint16_t *regs)
{
    int i;
    for (i = 0; i < 9; i++) {
        uint32_t u;
        memcpy(&u, &v[i], 4);
        uint8_t b[4] = { (uint8_t)(u >> 24), (uint8_t)(u >> 16),
                         (uint8_t)(u >> 8),  (uint8_t)u };   /* A B C D */
        uint16_t r0, r1;
        switch (order) {
            case MB_WORD_ABCD: r0 = (uint16_t)((b[0] << 8) | b[1]);
                               r1 = (uint16_t)((b[2] << 8) | b[3]); break;
            case MB_WORD_CDAB: r0 = (uint16_t)((b[2] << 8) | b[3]);
                               r1 = (uint16_t)((b[0] << 8) | b[1]); break;
            case MB_WORD_BADC: r0 = (uint16_t)((b[1] << 8) | b[0]);
                               r1 = (uint16_t)((b[3] << 8) | b[2]); break;
            default:           r0 = (uint16_t)((b[3] << 8) | b[2]);   /* DCBA */
                               r1 = (uint16_t)((b[1] << 8) | b[0]); break;
        }
        regs[2 * i]     = r0;
        regs[2 * i + 1] = r1;
    }
}

/* ---- decode：四种字序往返一致 ---- */
static void test_decode_orders(void)
{
    TEST_BEGIN("imu-decode-word-order");

    const float     v[9] = { 0.100f, -0.200f, 1.500f,      /* roll/pitch/yaw */
                             0.010f, 0.020f, 0.030f,      /* gx/gy/gz */
                             0.000f, 0.000f, 9.810f };    /* ax/ay/az */
    const mb_word_order_t orders[4] = {
        MB_WORD_ABCD, MB_WORD_CDAB, MB_WORD_BADC, MB_WORD_DCBA
    };

    int k;
    for (k = 0; k < 4; k++) {
        uint16_t regs[AGV_IMU_REG_COUNT];
        agv_imu_sample_t s;

        pack_regs(v, orders[k], regs);
        memset(&s, 0, sizeof(s));
        agv_imu_decode(regs, orders[k], &s);

        CHECK_EQ_DBL(s.roll,  0.100f, 1e-6);
        CHECK_EQ_DBL(s.pitch, -0.200f, 1e-6);
        CHECK_EQ_DBL(s.yaw,   1.500f, 1e-6);
        CHECK_EQ_DBL(s.gx,    0.010f, 1e-6);
        CHECK_EQ_DBL(s.gy,    0.020f, 1e-6);
        CHECK_EQ_DBL(s.gz,    0.030f, 1e-6);
        CHECK_EQ_DBL(s.ax,    0.000f, 1e-6);
        CHECK_EQ_DBL(s.ay,    0.000f, 1e-6);
        CHECK_EQ_DBL(s.az,    9.810f, 1e-6);
    }

    /* NULL 入参不崩、不写 */
    agv_imu_decode(NULL, MB_WORD_ABCD, NULL);
}

/* ---- 模块状态机：未启动 / 非法参数 / 设备打不开 ---- */
static void test_state_machine(void)
{
    TEST_BEGIN("imu-state-machine");

    agv_imu_sample_t s;

    /* 尚未启动：没有有效快照 */
    CHECK_EQ_I64(agv_imu_get(&s), 0);
    CHECK_EQ_I64(agv_imu_get(NULL), 0);
    CHECK_EQ_I64(agv_imu_offline(), 0);
    CHECK_EQ_I64((long long)agv_imu_stat_ok(), 0);
    CHECK_EQ_I64((long long)agv_imu_stat_err(), 0);

    /* 未启动时 stop 为无操作（幂等） */
    agv_imu_stop();
    agv_imu_stop();

    /* 参数非法：dev 为空 */
    CHECK_EQ_I64(agv_imu_start(NULL, NULL), MB_E_PARAM);

    /* 设备打不开：不存在的路径 → MB_E_IO，且不进入运行态 */
    CHECK_EQ_I64(agv_imu_start("/dev/agv_imu_no_such_dev", NULL), MB_E_IO);
    CHECK_EQ_I64(agv_imu_get(&s), 0);
    agv_imu_stop();     /* 失败后 stop 仍应是无操作 */
}

int main(void)
{
    test_decode_orders();
    test_state_machine();
    return TEST_SUMMARY();
}
