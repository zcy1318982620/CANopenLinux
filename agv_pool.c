/*
 * 工作线程池实现（见 agv_pool.h）。
 */

#include "agv_pool.h"

#include <stdlib.h>

struct agv_pool {
    pthread_t         *threads;     /* worker 线程句柄数组 */
    size_t             nthreads;
    agv_queue_t       *q;           /* 共享队列(调用方拥有) */
    agv_log_handler_t  handler;
    void              *user;
};

/* worker 主体：阻塞取记录 → 交给 handler；队列关闭且取空后退出。 */
static void *agv_worker(void *arg)
{
    agv_pool_t *p = (agv_pool_t *)arg;
    agv_logrec_t rec;

    while (agv_queue_pop(p->q, &rec) == 0) {
        p->handler(&rec, p->user);
    }
    return NULL;
}

agv_pool_t *agv_pool_create(size_t nthreads, agv_queue_t *q,
                            agv_log_handler_t handler, void *user)
{
    if (nthreads == 0 || q == NULL || handler == NULL) {
        return NULL;
    }

    agv_pool_t *p = (agv_pool_t *)calloc(1, sizeof(*p));
    if (p == NULL) {
        return NULL;
    }
    p->threads = (pthread_t *)calloc(nthreads, sizeof(pthread_t));
    if (p->threads == NULL) {
        free(p);
        return NULL;
    }
    p->nthreads = nthreads;
    p->q        = q;
    p->handler  = handler;
    p->user     = user;

    size_t i;
    for (i = 0; i < nthreads; i++) {
        if (pthread_create(&p->threads[i], NULL, agv_worker, p) != 0) {
            /* 回滚：关闭队列唤醒已建 worker，join 后释放 */
            agv_queue_close(p->q);
            size_t j;
            for (j = 0; j < i; j++) {
                (void)pthread_join(p->threads[j], NULL);
            }
            free(p->threads);
            free(p);
            return NULL;
        }
    }
    return p;
}

void agv_pool_destroy(agv_pool_t *p)
{
    if (p == NULL) {
        return;
    }

    agv_queue_close(p->q);          /* 先关再 join，否则 worker 卡在 cond_wait */

    /* join 的返回值：ESRCH(线程不存在)/EINVAL/EDEADLK(join 自己)均为调用方
     * bug，此处无法补救，且本函数返回 void；显式忽略并继续 join 其余线程。 */
    size_t i;
    for (i = 0; i < p->nthreads; i++) {
        (void)pthread_join(p->threads[i], NULL);
    }
    free(p->threads);
    free(p);
}
