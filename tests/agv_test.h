/*
 * 零框架单元测试最小骨架（见 docs/P2_ENGINEERING_TEST_KNOWLEDGE_print.html §1.5）。
 *
 * 为什么不引 Unity/CMocka：本工程测试对象极少（agv_queue、agv_kinematics），
 * 且要能直接 `make test` 一键跑，不希望额外拉第三方依赖。这里只用标准库 +
 * 一组宏：每个 CHECK* 累加断言数/失败数，测试文件末尾用 TEST_SUMMARY() 把
 * 失败数折算成进程退出码（0=全过），这样 Makefile / ctest 直接看退出码即可。
 *
 * 约定：每个测试文件是一个独立可执行程序（自带 main），只 include 本头一次。
 */

#ifndef AGV_TEST_H
#define AGV_TEST_H

#include <stdio.h>
#include <string.h>

static int         g_checks = 0;      /* 累计断言数 */
static int         g_fails  = 0;      /* 累计失败数 */
static const char *g_cur    = "";     /* 当前用例名(用于失败定位) */

#define TEST_BEGIN(name)                                          \
    do {                                                          \
        g_cur = (name);                                           \
        printf("[ RUN  ] %s\n", g_cur);                           \
    } while (0)

/* 布尔断言 */
#define CHECK(cond)                                               \
    do {                                                          \
        g_checks++;                                               \
        if (!(cond)) {                                            \
            g_fails++;                                            \
            printf("[FAIL  ] %s  %s:%d  %s\n",                    \
                   g_cur, __FILE__, __LINE__, #cond);             \
        }                                                         \
    } while (0)

/* 整数相等断言(统一按 long long 比较，省去各式 %d/%ld 烦恼) */
#define CHECK_EQ_I64(a, b)                                        \
    do {                                                          \
        long long _va = (long long)(a);                           \
        long long _vb = (long long)(b);                           \
        g_checks++;                                               \
        if (_va != _vb) {                                         \
            g_fails++;                                            \
            printf("[FAIL  ] %s  %s:%d  %s(%lld) != %s(%lld)\n",  \
                   g_cur, __FILE__, __LINE__, #a, _va, #b, _vb);  \
        }                                                         \
    } while (0)

/* 浮点相等断言(带绝对误差 eps) */
#define CHECK_EQ_DBL(a, b, eps)                                   \
    do {                                                          \
        double _va = (double)(a);                                 \
        double _vb = (double)(b);                                 \
        double _e  = (double)(eps);                               \
        g_checks++;                                               \
        if (!((_va - _vb) <= _e && (_vb - _va) <= _e)) {          \
            g_fails++;                                            \
            printf("[FAIL  ] %s  %s:%d  %s(%.12g) != %s(%.12g) "  \
                   "(eps %.3g)\n",                                \
                   g_cur, __FILE__, __LINE__, #a, _va, #b, _vb,   \
                   _e);                                           \
        }                                                         \
    } while (0)

/* 全部用例跑完后调用：打印汇总并给出进程退出码。 */
#define TEST_SUMMARY()                                            \
    (printf("\n==== %s: %d checks, %d failed ====\n",             \
            g_fails ? "FAIL" : "PASS", g_checks, g_fails),        \
     g_fails ? 1 : 0)

#endif /* AGV_TEST_H */
