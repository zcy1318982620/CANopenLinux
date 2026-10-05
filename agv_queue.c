/*
 * 有界环形队列实现（见 agv_queue.h）。
 */

#include "agv_queue.h"

#include <stdlib.h>
#include <string.h>

int agv_queue_init(agv_queue_t *q, size_t cap)
{
    if (q == NULL || cap < 2) {
        return -1;
    }

    /* 容量向上取整到 2 的幂，便于用 & (cap-1) 代替 % 取模 */
    size_t c = 1;
    while (c < cap) {
        c <<= 1;
    }

    memset(q, 0, sizeof(*q));
    q->buf = (agv_logrec_t *)calloc(c, sizeof(agv_logrec_t));
    if (q->buf == NULL) {
        return -1;
    }
    q->cap    = c;
    q->head   = 0;
    q->tail   = 0;
    q->count  = 0;
    q->closed = 0;
    atomic_store(&q->pushed, 0);
    atomic_store(&q->dropped, 0);

    if (pthread_mutex_init(&q->lock, NULL) != 0) {
        free(q->buf);
        q->buf = NULL;
        return -1;
    }
    if (pthread_cond_init(&q->not_empty, NULL) != 0) {
        (void)pthread_mutex_destroy(&q->lock);  /* 回滚，失败也无更优处理 */
        free(q->buf);
        q->buf = NULL;
        return -1;
    }
    return 0;
}

void agv_queue_destroy(agv_queue_t *q)
{
    if (q == NULL || q->buf == NULL) {
        return;
    }
    /* destroy 的返回值：EBUSY 说明还有线程在用(调用方 bug)，此处无法补救，
     * 且本函数返回 void，只能显式忽略并留下注释。 */
    (void)pthread_cond_destroy(&q->not_empty);
    (void)pthread_mutex_destroy(&q->lock);
    free(q->buf);
    q->buf = NULL;
}

int agv_queue_try_push(agv_queue_t *q, const agv_logrec_t *rec)
{
    if (q == NULL || q->buf == NULL || rec == NULL) {
        return -1;
    }

    /* 拿不到锁就丢：RT 线程绝不能在这里睡眠 */
    if (pthread_mutex_trylock(&q->lock) != 0) {
        atomic_fetch_add(&q->dropped, 1);
        return -1;
    }

    int rc;
    if (!q->closed && q->count < q->cap) {
        q->buf[q->tail] = *rec;
        q->tail = (q->tail + 1) & (q->cap - 1);
        q->count++;
        atomic_fetch_add(&q->pushed, 1);
        (void)pthread_cond_signal(&q->not_empty);  /* 无等待者时也无害 */
        rc = 0;
    } else {
        atomic_fetch_add(&q->dropped, 1);
        rc = -1;
    }

    /* unlock 失败说明锁状态已被破坏(调用方 bug)：数据未可靠记录，按失败返回 */
    if (pthread_mutex_unlock(&q->lock) != 0) {
        return -1;
    }
    return rc;
}

int agv_queue_pop(agv_queue_t *q, agv_logrec_t *out)
{
    if (q == NULL || q->buf == NULL || out == NULL) {
        return -1;
    }

    if (pthread_mutex_lock(&q->lock) != 0) {
        return -1;                                    /* 拿不到锁，无法取数据 */
    }
    while (q->count == 0 && !q->closed) {
        if (pthread_cond_wait(&q->not_empty, &q->lock) != 0) {
            (void)pthread_mutex_unlock(&q->lock);     /* wait 失败：退出前解锁 */
            return -1;
        }
    }
    if (q->count == 0 && q->closed) {                 /* 关闭且取空 → 退出 */
        (void)pthread_mutex_unlock(&q->lock);
        return -1;
    }

    *out = q->buf[q->head];
    q->head = (q->head + 1) & (q->cap - 1);
    q->count--;
    if (pthread_mutex_unlock(&q->lock) != 0) {
        return -1;
    }
    return 0;
}

void agv_queue_close(agv_queue_t *q)
{
    if (q == NULL || q->buf == NULL) {
        return;
    }
    if (pthread_mutex_lock(&q->lock) != 0) {
        return;                                       /* 拿不到锁无法安全置位 */
    }
    q->closed = 1;
    (void)pthread_cond_broadcast(&q->not_empty);  /* 唤醒全部消费者，否则 join 卡死 */
    (void)pthread_mutex_unlock(&q->lock);
}

uint64_t agv_queue_dropped(const agv_queue_t *q)
{
    if (q == NULL) {
        return 0;
    }
    return atomic_load(&q->dropped);
}
