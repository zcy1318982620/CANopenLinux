/*
 * 有界环形队列（生产者-消费者）—— RT 线程只入队、工作线程负责落盘。
 *
 * 设计要点（详见 CONCURRENCY_QUEUE_PRODUCER_CONSUMER_print.html）：
 *   - 容量固定(取 2 的幂)，环形数组，下标用 & (cap-1) 代替 % 取模；
 *   - 生产者(RT)走 agv_queue_try_push：pthread_mutex_trylock 拿不到锁 /
 *     队列满 / 已关闭，都立即丢弃并 dropped++，绝不阻塞 1ms 节拍线程；
 *   - 消费者(工作线程)走 agv_queue_pop：阻塞等待 + 条件变量，直到
 *     取到记录或队列关闭且为空；
 *   - 关闭用 agv_queue_close：置 closed 并 broadcast，唤醒所有消费者。
 */

#ifndef AGV_QUEUE_H
#define AGV_QUEUE_H

#include <stddef.h>
#include <stdint.h>
#include <stdatomic.h>
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 单条日志记录的最大文本长度(含结尾 '\0') */
#define AGV_LOG_TEXT_MAX 160

/* 一条日志记录：时间戳 + 级别 + 文本 */
typedef struct {
    uint64_t ts_us;                 /* CLOCK_MONOTONIC，微秒 */
    uint8_t  level;                 /* agv_log_level_t：0=DEBUG..4=FATAL */
    char     text[AGV_LOG_TEXT_MAX];
} agv_logrec_t;

typedef struct {
    agv_logrec_t    *buf;           /* 环形数组，容量 cap */
    size_t           cap;           /* 容量(元素数)，恒为 2 的幂 */
    size_t           head;          /* 出队下标 */
    size_t           tail;          /* 入队下标 */
    size_t           count;         /* 当前元素个数 */
    int              closed;        /* 1=已关闭，消费者取空后退出 */
    pthread_mutex_t  lock;          /* 保护 head/tail/count/closed */
    pthread_cond_t   not_empty;     /* 队列非空时唤醒消费者 */

    /* 计数器用原子量，供 RT 线程无锁读取(仅统计用) */
    _Atomic uint64_t pushed;        /* 累计成功入队数 */
    _Atomic uint64_t dropped;       /* 累计丢弃数(满/锁占用/已关闭) */
} agv_queue_t;

/*
 * 初始化队列。cap 会向上取整到 2 的幂；小于 2 视为非法。
 * @return 0 成功，-1 失败(参数非法或内存/锁初始化失败)。
 */
int  agv_queue_init(agv_queue_t *q, size_t cap);

/* 销毁队列(调用前必须确保已无生产者在入队、消费者已退出)。 */
void agv_queue_destroy(agv_queue_t *q);

/*
 * 非阻塞入队：拿不到锁/满/已关闭 → dropped++ 并返回 -1；成功返回 0。
 * 专供 RT 线程使用，保证不睡眠。
 */
int  agv_queue_try_push(agv_queue_t *q, const agv_logrec_t *rec);

/*
 * 阻塞出队：成功返回 0 并写出记录；队列已关闭且为空时返回 -1(消费者退出)。
 */
int  agv_queue_pop(agv_queue_t *q, agv_logrec_t *out);

/* 关闭队列：置 closed 并唤醒所有等待中的消费者(配合 join 用)。 */
void agv_queue_close(agv_queue_t *q);

/* 当前累计丢弃数。 */
uint64_t agv_queue_dropped(const agv_queue_t *q);

#ifdef __cplusplus
}
#endif

#endif /* AGV_QUEUE_H */
