/*
 * 统一日志模块实现（见 agv_log.h）。
 */

#include "agv_log.h"
#include "agv_queue.h"
#include "agv_pool.h"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>

/* 全局单例：本工程只开一条日志通道 */
static agv_queue_t  g_q;        /* 有界环形队列(生产者=各线程，消费者=worker) */
static agv_pool_t  *g_pool;     /* 工作线程池 */
static int          g_ready;    /* 1=已就绪 */

/* 级别 → 单字符标签，与 syslog 习惯一致 */
static char agv_log_tag(agv_log_level_t level)
{
    switch (level) {
    case AGV_LOG_DEBUG: return 'D';
    case AGV_LOG_INFO:  return 'I';
    case AGV_LOG_WARN:  return 'W';
    case AGV_LOG_ERROR: return 'E';
    case AGV_LOG_FATAL: return 'F';
    default:            return '?';
    }
}

/* worker 回调：把一条记录写到 stdout。
 * 这里是"最后一道"，写失败已无处上报，故显式忽略返回值。 */
static void agv_log_sink(const agv_logrec_t *rec, void *user)
{
    (void)user;
    (void)fputs(rec->text, stdout);
    (void)fputc('\n', stdout);
    (void)fflush(stdout);
}

int agv_log_init(size_t cap, size_t nthreads)
{
    if (g_ready) {
        return 0;
    }
    if (agv_queue_init(&g_q, cap) != 0) {
        return -1;
    }
    g_pool = agv_pool_create(nthreads, &g_q, agv_log_sink, NULL);
    if (g_pool == NULL) {
        agv_queue_destroy(&g_q);
        return -1;
    }
    g_ready = 1;
    return 0;
}

void agv_log_deinit(void)
{
    if (g_pool != NULL) {
        agv_pool_destroy(g_pool);
        g_pool = NULL;
    }
    if (g_ready) {
        agv_queue_destroy(&g_q);
        g_ready = 0;
    }
}

/* 组装一条记录(补时间戳 + 级别 + 文本)并尝试非阻塞入队。
 * text 为已格式化正文；file==NULL 表示不含位置信息。 */
static int agv_log_push(agv_log_level_t level, const char *file, int line,
                        const char *text)
{
    agv_logrec_t rec;
    memset(&rec, 0, sizeof(rec));
    rec.level = (uint8_t)level;

    /* 时间戳：取不到就置 0，不影响日志正文 */
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0) {
        rec.ts_us = (uint64_t)ts.tv_sec * 1000000ull
                  + (uint64_t)(ts.tv_nsec / 1000);
    } else {
        rec.ts_us = 0;
    }

    if (file != NULL) {
        /* 只保留文件名，去掉 __FILE__ 可能带的目录前缀 */
        const char *name = strrchr(file, '/');
        name = (name != NULL) ? (name + 1) : file;
        (void)snprintf(rec.text, sizeof(rec.text), "[%c] %s:%d %s",
                       agv_log_tag(level), name, line, text);
    } else {
        (void)snprintf(rec.text, sizeof(rec.text), "[%c] %s",
                       agv_log_tag(level), text);
    }

    return agv_queue_try_push(&g_q, &rec);
}

int agv_log_write(agv_log_level_t level, const char *file, int line,
                  const char *fmt, ...)
{
    if (!g_ready) {
        return -1;
    }

    /* 先格式化正文，再交给 agv_log_push 拼成定长记录 */
    char body[AGV_LOG_TEXT_MAX];
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(body, sizeof(body), fmt, ap);
    va_end(ap);
    if (n < 0) {
        return -1;                      /* 格式化失败 */
    }
    /* n >= sizeof(body) 表示正文被截断，仍照常输出(定长记录的取舍) */

    return agv_log_push(level, file, line, body);
}

int agv_log_write_text(agv_log_level_t level, const char *text)
{
    if (!g_ready) {
        return -1;
    }
    return agv_log_push(level, NULL, 0, (text != NULL) ? text : "");
}

uint64_t agv_log_dropped(void)
{
    return agv_queue_dropped(&g_q);
}
