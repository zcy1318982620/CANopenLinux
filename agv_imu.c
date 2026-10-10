/*
 * IMU 采集模块实现（见 agv_imu.h）。
 *
 * 关键点：
 *   - 一次事务读满 18 个输入寄存器(0x04)，再在本地 decode 成 9 个 float32；
 *     不做 9 次单点读(那样 9 倍帧开销，且字序/切帧重复)。
 *   - 跨线程只传"最新快照"：采集线程用 seqlock(序号奇偶)发布，RT 拍无锁读，
 *     避免在实时路径上引入可睡眠的互斥量。
 *   - 本模块不打印日志；调用方读 agv_imu_stat_ok/err 与 agv_imu_offline 决定。
 */

#include "agv_imu.h"

#include <pthread.h>
#include <stdatomic.h>
#include <string.h>
#include <time.h>

/* ---- 模块状态（单实例：一条串口） ---- */
static mb_ctx_t        *s_ctx;        /* Modbus 主站句柄 */
static pthread_t        s_tid;
static _Atomic int      s_run;        /* 1=采集线程运行中 */
static mb_word_order_t  s_order;      /* decode 用字序(从 cfg 拷入) */

/* 最新快照 + seqlock 序号：偶=稳定，奇=写入中 */
static agv_imu_sample_t s_snap;
static _Atomic uint32_t s_seq;
static _Atomic int      s_valid;      /* 1=至少发布过一帧 */

/* 统计/离线：线程内更新，外部只读 */
static _Atomic uint64_t s_ok;
static _Atomic uint64_t s_err;
static _Atomic int      s_consec_fail;   /* 连续失败次数，成功清零 */

/* ---------------------------------------------------------------------- */
/* 纯函数：18 个寄存器 → 9 个 float32                                       */
/* ---------------------------------------------------------------------- */
void agv_imu_decode(const uint16_t *regs, mb_word_order_t order,
                    agv_imu_sample_t *out)
{
    if (regs == NULL || out == NULL) {
        return;
    }

    float v[9];
    int i;
    for (i = 0; i < 9; i++) {
        v[i] = mb_regs_to_float32(regs[2 * i], regs[2 * i + 1], order);
    }

    out->roll  = v[0]; out->pitch = v[1]; out->yaw = v[2];
    out->gx    = v[3]; out->gy    = v[4]; out->gz  = v[5];
    out->ax    = v[6]; out->ay    = v[7]; out->az  = v[8];
}

/* ---------------------------------------------------------------------- */
/* seqlock 发布/读取                                                        */
/* ---------------------------------------------------------------------- */
static void imu_publish(const agv_imu_sample_t *s)
{
    uint32_t seq = atomic_load_explicit(&s_seq, memory_order_relaxed);

    atomic_store_explicit(&s_seq, seq + 1u, memory_order_relaxed); /* → 奇数 */
    atomic_thread_fence(memory_order_release);
    s_snap = *s;
    atomic_thread_fence(memory_order_release);
    atomic_store_explicit(&s_seq, seq + 2u, memory_order_relaxed); /* → 偶数 */
    atomic_store_explicit(&s_valid, 1, memory_order_release);
}

int agv_imu_get(agv_imu_sample_t *out)
{
    if (out == NULL || !atomic_load_explicit(&s_valid, memory_order_acquire)) {
        return 0;
    }

    for (;;) {
        uint32_t s1 = atomic_load_explicit(&s_seq, memory_order_acquire);
        if ((s1 & 1u) != 0u) {
            continue;                       /* 写者正在更新，重试 */
        }
        *out = s_snap;                      /* 可能撕裂，靠 seq 复检 */
        atomic_thread_fence(memory_order_acquire);
        uint32_t s2 = atomic_load_explicit(&s_seq, memory_order_relaxed);
        if (s1 == s2) {
            break;                          /* 读期间无写入，快照一致 */
        }
    }

    return 1;
}

/* ---------------------------------------------------------------------- */
/* 采集线程                                                                 */
/* ---------------------------------------------------------------------- */
static void *imu_worker(void *arg)
{
    (void)arg;

    uint16_t regs[AGV_IMU_REG_COUNT];
    agv_imu_sample_t s;

    while (atomic_load_explicit(&s_run, memory_order_relaxed)) {
        int r = mb_read_regs(s_ctx, MB_FC_READ_INPUT,
                             AGV_IMU_REG_BASE, AGV_IMU_REG_COUNT, regs);
        if (r == (int)AGV_IMU_REG_COUNT) {
            agv_imu_decode(regs, s_order, &s);
            imu_publish(&s);
            atomic_store_explicit(&s_consec_fail, 0, memory_order_relaxed);
            atomic_fetch_add_explicit(&s_ok, 1u, memory_order_relaxed);
        }
        else {
            atomic_fetch_add_explicit(&s_err, 1u, memory_order_relaxed);
            int cf = atomic_load_explicit(&s_consec_fail, memory_order_relaxed);
            if (cf < MB_OFFLINE_THRESHOLD) {
                atomic_store_explicit(&s_consec_fail, cf + 1, memory_order_relaxed);
            }
        }

        struct timespec ts = {0, (long)AGV_IMU_POLL_MS * 1000000L};
        nanosleep(&ts, NULL);
    }

    return NULL;
}

/* ---------------------------------------------------------------------- */
/* 启停                                                                     */
/* ---------------------------------------------------------------------- */
int agv_imu_start(const char *dev, const mb_cfg_t *cfg)
{
    if (dev == NULL || s_ctx != NULL) {
        return MB_E_PARAM;
    }

    mb_cfg_t c;
    if (cfg != NULL) {
        c = *cfg;
    }
    else {
        MB_CFG_DEFAULT_INIT(c);
    }
    s_order = c.word_order;

    s_ctx = mb_open(dev, &c);
    if (s_ctx == NULL) {
        return MB_E_IO;
    }

    atomic_store(&s_valid, 0);
    atomic_store(&s_ok, 0);
    atomic_store(&s_err, 0);
    atomic_store(&s_consec_fail, 0);
    atomic_store(&s_seq, 0);
    atomic_store(&s_run, 1);

    if (pthread_create(&s_tid, NULL, imu_worker, NULL) != 0) {
        atomic_store(&s_run, 0);
        mb_close(s_ctx);
        s_ctx = NULL;
        return MB_E_IO;
    }

    return 0;
}

void agv_imu_stop(void)
{
    if (s_ctx == NULL) {
        return;
    }

    atomic_store(&s_run, 0);
    pthread_join(s_tid, NULL);
    mb_close(s_ctx);
    s_ctx = NULL;
}

int agv_imu_offline(void)
{
    return atomic_load_explicit(&s_consec_fail, memory_order_relaxed)
           >= MB_OFFLINE_THRESHOLD;
}

uint64_t agv_imu_stat_ok(void)
{
    return atomic_load_explicit(&s_ok, memory_order_relaxed);
}

uint64_t agv_imu_stat_err(void)
{
    return atomic_load_explicit(&s_err, memory_order_relaxed);
}
