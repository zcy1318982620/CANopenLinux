/*
 * agv_queue 单元测试（用例表见 docs/P2_ENGINEERING_TEST_KNOWLEDGE_print.html §1.6）。
 *
 * 覆盖：容量取幂 / 非法参数 / FIFO 回绕 / 满队丢帧 / 关闭语义 /
 *       数据完整性(memcmp) / 并发守恒(不丢不重 + FIFO 单调)。
 */

#include "agv_queue.h"
#include "agv_test.h"

#include <pthread.h>
#include <stdatomic.h>
#include <stdint.h>

/* ---------- 用例 1：容量向上取整到 2 的幂 ---------- */
static void test_cap_roundup(void)
{
    TEST_BEGIN("queue/cap-roundup");
    agv_queue_t q;

    CHECK_EQ_I64(agv_queue_init(&q, 3), 0);
    CHECK_EQ_I64((long long)q.cap, 4);         /* 3 → 4 */
    agv_queue_destroy(&q);

    CHECK_EQ_I64(agv_queue_init(&q, 4), 0);
    CHECK_EQ_I64((long long)q.cap, 4);         /* 已取幂保持不变 */
    agv_queue_destroy(&q);

    CHECK_EQ_I64(agv_queue_init(&q, 5), 0);
    CHECK_EQ_I64((long long)q.cap, 8);         /* 5 → 8 */
    agv_queue_destroy(&q);
}

/* ---------- 用例 2：非法参数应安全失败 ---------- */
static void test_bad_args(void)
{
    TEST_BEGIN("queue/bad-args");
    agv_queue_t q;

    CHECK_EQ_I64(agv_queue_init(&q, 1), -1);   /* cap < 2 非法 */
    CHECK_EQ_I64(agv_queue_init(&q, 0), -1);
    CHECK_EQ_I64(agv_queue_init(NULL, 8), -1);

    agv_logrec_t rec;
    memset(&rec, 0, sizeof(rec));
    CHECK_EQ_I64(agv_queue_try_push(NULL, &rec), -1);
    CHECK_EQ_I64(agv_queue_pop(NULL, &rec), -1);

    CHECK_EQ_I64(agv_queue_init(&q, 4), 0);
    CHECK_EQ_I64(agv_queue_try_push(&q, NULL), -1);
    CHECK_EQ_I64(agv_queue_pop(&q, NULL), -1);
    CHECK_EQ_I64((long long)agv_queue_dropped(NULL), 0);
    agv_queue_destroy(&q);

    /* 对 NULL 的 destroy/close 必须无害 */
    agv_queue_destroy(NULL);
    agv_queue_close(NULL);
}

/* ---------- 用例 3：FIFO 顺序 + head/tail 回绕 ---------- */
static void test_fifo_wrap(void)
{
    TEST_BEGIN("queue/fifo-wrap");
    agv_queue_t q;
    agv_queue_init(&q, 4);                     /* 实际 cap=4 */

    /* 反复「塞满 → 取空」，让 head/tail 多次越过数组末尾，走回绕分支 */
    for (int round = 0; round < 3; round++) {
        for (int i = 0; i < 4; i++) {
            agv_logrec_t r;
            memset(&r, 0, sizeof(r));
            r.ts_us = (uint64_t)(round * 4 + i);
            CHECK_EQ_I64(agv_queue_try_push(&q, &r), 0);
        }
        for (int i = 0; i < 4; i++) {
            agv_logrec_t out;
            CHECK_EQ_I64(agv_queue_pop(&q, &out), 0);
            CHECK_EQ_I64((long long)out.ts_us, (long long)(round * 4 + i));
        }
    }
    CHECK_EQ_I64((long long)agv_queue_dropped(&q), 0);
    agv_queue_destroy(&q);
}

/* ---------- 用例 4：满队丢帧，dropped++，腾位后可续入 ---------- */
static void test_full_drop(void)
{
    TEST_BEGIN("queue/full-drop");
    agv_queue_t q;
    agv_queue_init(&q, 4);                     /* cap=4 */

    for (int i = 0; i < 4; i++) {
        agv_logrec_t r;
        memset(&r, 0, sizeof(r));
        r.ts_us = (uint64_t)i;
        CHECK_EQ_I64(agv_queue_try_push(&q, &r), 0);
    }

    agv_logrec_t r;
    memset(&r, 0, sizeof(r));
    CHECK_EQ_I64(agv_queue_try_push(&q, &r), -1);   /* 第 5 条：满 */
    CHECK_EQ_I64(agv_queue_try_push(&q, &r), -1);   /* 第 6 条：满 */
    CHECK_EQ_I64((long long)agv_queue_dropped(&q), 2);

    /* 取走一条后应出现空位，可再入队且不再增加 dropped */
    agv_logrec_t out;
    CHECK_EQ_I64(agv_queue_pop(&q, &out), 0);
    r.ts_us = 999;
    CHECK_EQ_I64(agv_queue_try_push(&q, &r), 0);
    CHECK_EQ_I64((long long)agv_queue_dropped(&q), 2);
    agv_queue_destroy(&q);
}

/* ---------- 用例 5：关闭语义 ---------- */
static void test_close_semantics(void)
{
    TEST_BEGIN("queue/close");
    agv_queue_t q;
    agv_queue_init(&q, 4);

    agv_logrec_t r;
    memset(&r, 0, sizeof(r));
    r.ts_us = 7;
    agv_queue_try_push(&q, &r);
    agv_queue_try_push(&q, &r);

    agv_queue_close(&q);

    /* 关闭后入队一律丢弃 */
    CHECK_EQ_I64(agv_queue_try_push(&q, &r), -1);
    CHECK_EQ_I64((long long)agv_queue_dropped(&q), 1);

    /* 关闭但非空：仍应把已有数据取完，之后才返回 -1(退出) */
    agv_logrec_t out;
    CHECK_EQ_I64(agv_queue_pop(&q, &out), 0);
    CHECK_EQ_I64(agv_queue_pop(&q, &out), 0);
    CHECK_EQ_I64(agv_queue_pop(&q, &out), -1);
    agv_queue_destroy(&q);
}

/* ---------- 用例 6：数据完整性（整条记录 memcmp） ---------- */
static void test_data_integrity(void)
{
    TEST_BEGIN("queue/data-integrity");
    agv_queue_t q;
    agv_queue_init(&q, 8);

    agv_logrec_t in;
    memset(&in, 0, sizeof(in));
    in.ts_us = 0x1122334455667788ULL;
    in.level = 3;
    for (size_t i = 0; i + 1 < sizeof(in.text); i++) {
        in.text[i] = (char)(i * 7 + 1);
    }
    in.text[sizeof(in.text) - 1] = '\0';

    CHECK_EQ_I64(agv_queue_try_push(&q, &in), 0);

    agv_logrec_t out;
    memset(&out, 0xAA, sizeof(out));           /* 先污染，确认确实被覆盖 */
    CHECK_EQ_I64(agv_queue_pop(&q, &out), 0);
    CHECK_EQ_I64(memcmp(&in, &out, sizeof(in)), 0);
    agv_queue_destroy(&q);
}

/* ---------- 用例 7：并发守恒（不丢不重 + FIFO 单调） ---------- */
typedef struct {
    agv_queue_t *q;
    uint64_t     total;         /* 生产者尝试入队总数 */
    uint64_t     ok_pushes;     /* 实际成功入队数 */
} producer_arg_t;

typedef struct {
    agv_queue_t *q;
    uint64_t     seen;          /* 消费者取到条数 */
    int          monotonic;     /* 1=序列严格递增(证明无重复/乱序) */
} consumer_arg_t;

static void *producer_fn(void *p)
{
    producer_arg_t *a = (producer_arg_t *)p;
    for (uint64_t i = 0; i < a->total; i++) {
        agv_logrec_t r;
        memset(&r, 0, sizeof(r));
        r.ts_us = i;                            /* 用序号当时间戳 */
        if (agv_queue_try_push(a->q, &r) == 0) {
            a->ok_pushes++;
        }
    }
    return NULL;
}

static void *consumer_fn(void *p)
{
    consumer_arg_t *a = (consumer_arg_t *)p;
    agv_logrec_t out;
    uint64_t last  = 0;
    int      first = 1;
    while (agv_queue_pop(a->q, &out) == 0) {
        if (!first && out.ts_us <= last) {
            a->monotonic = 0;
        }
        last  = out.ts_us;
        first = 0;
        a->seen++;
    }
    return NULL;
}

static void test_concurrent(void)
{
    TEST_BEGIN("queue/concurrent");
    const uint64_t TOTAL = 20000;               /* 远超容量，必然触发 dropped */
    agv_queue_t q;
    agv_queue_init(&q, 256);

    producer_arg_t pa = { &q, TOTAL, 0 };
    consumer_arg_t ca = { &q, 0, 1 };

    pthread_t tc, tp;
    pthread_create(&tc, NULL, consumer_fn, &ca);   /* 先起消费者，避免早期丢太多 */
    pthread_create(&tp, NULL, producer_fn, &pa);

    pthread_join(tp, NULL);
    agv_queue_close(&q);                        /* 唤醒可能阻塞的消费者 */
    pthread_join(tc, NULL);

    uint64_t pushed  = atomic_load(&q.pushed);
    uint64_t dropped = agv_queue_dropped(&q);

    CHECK_EQ_I64((long long)pa.ok_pushes, (long long)pushed);         /* 成功数一致 */
    CHECK_EQ_I64((long long)(pushed + dropped), (long long)TOTAL);    /* 守恒：不丢不重 */
    CHECK_EQ_I64((long long)ca.seen, (long long)pushed);              /* 取尽 */
    CHECK_EQ_I64(ca.monotonic, 1);                                    /* FIFO 有序 */
    agv_queue_destroy(&q);
}

int main(void)
{
    printf("==== test_agv_queue ====\n");
    test_cap_roundup();
    test_bad_args();
    test_fifo_wrap();
    test_full_drop();
    test_close_semantics();
    test_data_integrity();
    test_concurrent();
    return TEST_SUMMARY();
}
