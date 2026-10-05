/*
 * 统一日志模块 —— 分级(5 级) + 异步落盘(RT 安全)。
 *
 * 设计要点（详见 LOG_ERROR_HANDLING_print.html §2）：
 *   - 级别数值越大越严重：DEBUG < INFO < WARN < ERROR < FATAL；
 *   - 编译期过滤：低于 AGV_LOG_MIN_LEVEL 的日志不生成代码，可用 -D 覆盖；
 *   - 运行期非阻塞：格式化后 try_push 进有界队列，由工作线程落 stdout；
 *     队列满 / 锁被占则丢弃(dropped++)，绝不拖慢 RT 线程；
 *   - 用法：AGV_LOGE("open %s failed: %s", path, strerror(errno));
 *
 * 分层原则：本模块依赖 agv_queue + agv_pool；反过来 agv_queue/agv_pool
 * 不依赖日志(底层只返回错误码、不打印)，避免循环依赖与层层刷屏。
 */

#ifndef AGV_LOG_H
#define AGV_LOG_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 日志级别：数值越大越严重 */
typedef enum {
    AGV_LOG_DEBUG = 0,
    AGV_LOG_INFO  = 1,
    AGV_LOG_WARN  = 2,
    AGV_LOG_ERROR = 3,
    AGV_LOG_FATAL = 4
} agv_log_level_t;

/*
 * 编译期最低级别：低于该级别的日志在编译期被整体消除。
 * 例如 make OPT="-DAGV_LOG_MIN_LEVEL=AGV_LOG_DEBUG" 可打开全部日志。
 */
#ifndef AGV_LOG_MIN_LEVEL
#define AGV_LOG_MIN_LEVEL AGV_LOG_INFO
#endif

/*
 * 初始化日志：创建有界队列(cap，向上取 2 的幂)与 nthreads 个工作线程。
 * 可重复调用，已初始化时直接返回 0。
 * @return 0 成功；-1 失败(队列或线程池创建失败)。
 */
int  agv_log_init(size_t cap, size_t nthreads);

/* 关闭日志：先关队列唤醒 worker，再 join，最后释放。 */
void agv_log_deinit(void);

/*
 * 写入一条日志(非阻塞)。file/line 一般由 AGV_LOG 宏自动带入。
 * @return 0 已入队；-1 未就绪 / 格式化失败 / 队列满被丢弃。
 */
int  agv_log_write(agv_log_level_t level, const char *file, int line,
                   const char *fmt, ...);

/*
 * 写入一条"已格式化"的文本(供 log_printf 适配器转发用，见 CO_main_basic.c)。
 * 与 agv_log_write 的区别：不再做 printf 风格格式化，也不带文件名/行号，
 * 输出形如 "[级别] 正文"。
 * @return 0 已入队；-1 未就绪 / 队列满被丢弃。
 */
int  agv_log_write_text(agv_log_level_t level, const char *text);

/* 当前累计丢弃数(队列满 / 锁被占导致)，供 RT 侧观测。 */
uint64_t agv_log_dropped(void);

/*
 * 分级日志宏：自动带文件名/行号；低于编译门限的调用整体消失。
 * 放在 do{...}while(0) 里，保证在 if/else 中当单条语句使用也安全。
 */
#define AGV_LOG(level, ...)                                                 \
    do {                                                                    \
        if ((level) >= AGV_LOG_MIN_LEVEL) {                                 \
            (void)agv_log_write((level), __FILE__, __LINE__, __VA_ARGS__);  \
        }                                                                   \
    } while (0)

#define AGV_LOGD(...) AGV_LOG(AGV_LOG_DEBUG, __VA_ARGS__)
#define AGV_LOGI(...) AGV_LOG(AGV_LOG_INFO,  __VA_ARGS__)
#define AGV_LOGW(...) AGV_LOG(AGV_LOG_WARN,  __VA_ARGS__)
#define AGV_LOGE(...) AGV_LOG(AGV_LOG_ERROR, __VA_ARGS__)
#define AGV_LOGF(...) AGV_LOG(AGV_LOG_FATAL, __VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif /* AGV_LOG_H */
