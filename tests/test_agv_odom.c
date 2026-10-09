/*
 * agv_kinematics 单元测试（用例表见 docs/P2_ENGINEERING_TEST_KNOWLEDGE_print.html §1.7）。
 *
 * 覆盖正解：零增量(首拍) / 直行 / 原地转 / 一整圈标定 / 32 位回绕；
 *      逆解：基准值(6366) / 转向 / 空指针保护。
 * 可测性来源：积分公式已抽为纯函数 agv_odom_step（见 agv_kinematics.h）。
 */

#include "agv_kinematics.h"
#include "agv_test.h"

#include <math.h>
#include <stdint.h>

#define K        ODOM_K_PER_COUNT
#define B        ODOM_WHEEL_BASE_M
#define TWO_PI_R (2.0 * ODOM_PI * ODOM_WHEEL_R_M)   /* 一整圈轮面位移 = 2πr */

/* ---------- 正解 1：零增量(对应"首拍只记起点、不积分") ---------- */
static void test_zero_delta(void)
{
    TEST_BEGIN("odom/zero-delta");
    double x = 0.0, y = 0.0, th = 0.0;
    agv_odom_step(123, -456, 123, -456, &x, &y, &th);
    CHECK_EQ_DBL(x, 0.0, 1e-12);
    CHECK_EQ_DBL(y, 0.0, 1e-12);
    CHECK_EQ_DBL(th, 0.0, 1e-12);
}

/* ---------- 正解 2：直行(两轮同向同量 → θ 不变，y 不变) ---------- */
static void test_straight(void)
{
    TEST_BEGIN("odom/straight");
    double x = 0.0, y = 0.0, th = 0.0;
    int32_t c = 1000;
    agv_odom_step(0, 0, c, c, &x, &y, &th);
    CHECK_EQ_DBL(x,  K * (double)c, 1e-12);
    CHECK_EQ_DBL(y,  0.0, 1e-12);
    CHECK_EQ_DBL(th, 0.0, 1e-12);
}

/* ---------- 正解 3：原地转(右进左退等量 → 位移≈0，仅 θ 变) ---------- */
static void test_spin_in_place(void)
{
    TEST_BEGIN("odom/spin-in-place");
    double x = 0.0, y = 0.0, th = 0.0;
    int32_t c = 500;
    agv_odom_step(0, 0, c, -c, &x, &y, &th);   /* 右轮 +c，左轮 -c */
    CHECK_EQ_DBL(x,  0.0, 1e-12);
    CHECK_EQ_DBL(y,  0.0, 1e-12);
    CHECK_EQ_DBL(th, (2.0 * K * (double)c) / B, 1e-12);
}

/* ---------- 正解 4：标定(轮子转满 N 计数 → 前进 2πr) ---------- */
static void test_one_rev_calibration(void)
{
    TEST_BEGIN("odom/one-rev-calibration");
    double x = 0.0, y = 0.0, th = 0.0;
    agv_odom_step(0, 0, ODOM_COUNT_PER_REV, ODOM_COUNT_PER_REV, &x, &y, &th);
    CHECK_EQ_DBL(x,  TWO_PI_R, 1e-9);
    CHECK_EQ_DBL(th, 0.0, 1e-12);
}

/* ---------- 正解 5：32 位计数器回绕(0x7FFFFFFF → 0x80000000 增量为 1) ---------- */
static void test_wraparound(void)
{
    TEST_BEGIN("odom/wraparound-32bit");
    double x = 0.0, y = 0.0, th = 0.0;
    int32_t prev = INT32_MAX;                       /* 0x7FFFFFFF */
    int32_t cur  = (int32_t)(uint32_t)0x80000000u;  /* INT32_MIN，索引越回绕 */
    agv_odom_step(prev, prev, cur, cur, &x, &y, &th);
    CHECK_EQ_DBL(x,  K * 1.0, 1e-12);               /* 应只有 1 个计数的增量 */
    CHECK_EQ_DBL(y,  0.0, 1e-12);
    CHECK_EQ_DBL(th, 0.0, 1e-12);
}

/* ---------- 逆解 1：基准值 v=0.2 m/s，ω=0 → 两轮均 6366 counts/s ---------- */
static void test_ik_baseline(void)
{
    TEST_BEGIN("ik/baseline-0.2ms");
    int32_t vR = 0, vL = 0;
    agv_inverse_kinematics(0.2, 0.0, &vR, &vL);
    CHECK_EQ_I64(vR, 6366);
    CHECK_EQ_I64(vL, 6366);
}

/* ---------- 逆解 2：直行+转向(v=0.2, ω=1) ---------- */
static void test_ik_turn(void)
{
    TEST_BEGIN("ik/turn");
    int32_t vR = 0, vL = 0;
    agv_inverse_kinematics(0.2, 1.0, &vR, &vL);
    /* vR_ms=0.2+1*0.15=0.35 → 11140；vL_ms=0.2-0.15=0.05 → 1591 */
    CHECK_EQ_I64(vR, 11140);
    CHECK_EQ_I64(vL, 1591);
}

/* ---------- 逆解 3：空指针保护(可单独只要一路) ---------- */
static void test_ik_null_guard(void)
{
    TEST_BEGIN("kinematics/null-guard");
    /* 正解三指针全空：应直接返回、不崩 */
    agv_odom_step(0, 0, 10, 10, NULL, NULL, NULL);

    /* 逆解只写左轮 */
    int32_t v = 12345;
    agv_inverse_kinematics(0.2, 0.0, NULL, &v);
    CHECK_EQ_I64(v, 6366);
}

int main(void)
{
    printf("==== test_agv_odom ====\n");
    test_zero_delta();
    test_straight();
    test_spin_in_place();
    test_one_rev_calibration();
    test_wraparound();
    test_ik_baseline();
    test_ik_turn();
    test_ik_null_guard();
    return TEST_SUMMARY();
}
