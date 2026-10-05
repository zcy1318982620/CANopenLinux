/*
 * 工作线程池 —— 一组固定数量的消费者线程，从 agv_queue_t 里取记录并处理。
 *
 * 与 CONCURRENCY_THREADPOOL_print.html §4 的通用任务池(pool_add(fn,arg))相比，
 * 这里是它的"常驻消费者"形态：worker 不取一次性任务，而是循环 pop 队列，
 * 对每条记录调用同一个 handler。日志/上报这种"源源不断的同类工作"用它最合适。
 *
 * 关闭顺序（关键）：agv_pool_destroy → agv_queue_close(broadcast) → join 全部
 * worker → 释放。必须先 close 再 join，否则 worker 还在 cond_wait，join 会卡死。
 */

#ifndef AGV_POOL_H
#define AGV_POOL_H

#include <stddef.h>

#include "agv_queue.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 每条记录的处理回调；在某个 worker 线程里执行。 */
typedef void (*agv_log_handler_t)(const agv_logrec_t *rec, void *user);

typedef struct agv_pool agv_pool_t;

/*
 * 创建线程池：启动 nthreads 个 worker，共同消费队列 q，
 * 每条记录交给 handler(rec, user) 处理。
 * @return 池对象；参数非法或创建失败返回 NULL(已回滚创建出的线程)。
 */
agv_pool_t *agv_pool_create(size_t nthreads, agv_queue_t *q,
                            agv_log_handler_t handler, void *user);

/*
 * 优雅关闭：先 close 队列(唤醒并排空)，再 join 所有 worker，最后释放。
 * 调用前应确保生产者(RT 线程)已停止入队。
 */
void agv_pool_destroy(agv_pool_t *p);

#ifdef __cplusplus
}
#endif

#endif /* AGV_POOL_H */
