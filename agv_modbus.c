/*
 * Modbus RTU 主站实现（RS485 半双工）。
 *
 * 分层：本文件不依赖 agv_log / CANopen，纯 POSIX 串口 + 返回码，
 *       以便 test_agv_modbus 单独链接做纯函数单测。
 *
 * 收发时序（半双工，学习册 §9）：
 *   ① 静默 >= t3.5（从站靠总线静默切帧）
 *   ② 切"发" → write() → tcdrain() 等最后一个字节真正移出 → 切"收"
 *   ③ poll() 等响应，超时 → 重试
 *   ④ 校验 CRC → 查异常位 → 解数据
 * RS485 方向：优先让内核接管(TIOCSRS485)；内核不支持时兜底手动翻 RTS。
 */

#include "agv_modbus.h"

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>
#include <sys/ioctl.h>

#ifdef __linux__
#include <linux/serial.h>
#endif

/* 同一帧内字节间隔超时(ms)：略大于最慢波特率(9600)下的字符时间(~1.2ms) */
#define MB_INTERCHAR_MS   20
/* RTU 单帧最大长度：地址+功能码+最多 252 数据字节+CRC */
#define MB_MAX_ADU        256

struct mb_ctx {
    int             fd;
    uint8_t         slave;
    int             baud;
    int             timeout_ms;
    int             retries;
    int             rs485;
    int             rs485_kernel;   /* 1=内核接管方向控制 */
    mb_word_order_t word_order;
    /* 统计与离线判定 */
    uint64_t        stat_ok;
    uint64_t        stat_err;
    int             consec_fail;
};

/* ---------------- 纯函数 ---------------- */

uint16_t mb_crc16(const uint8_t *buf, size_t len)
{
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < len; i++) {
        crc ^= (uint16_t)buf[i];
        for (int b = 0; b < 8; b++) {
            if (crc & 0x0001)
                crc = (uint16_t)((crc >> 1) ^ 0xA001);
            else
                crc = (uint16_t)(crc >> 1);
        }
    }
    return crc;
}

/* 在 out[0..len-1] 之后追加 CRC（低字节先发），返回 len+2 */
static int mb_append_crc(uint8_t *out, int len)
{
    uint16_t crc = mb_crc16(out, (size_t)len);
    out[len]     = (uint8_t)(crc & 0xFF);        /* 低字节先发 */
    out[len + 1] = (uint8_t)((crc >> 8) & 0xFF);
    return len + 2;
}

int mb_build_read(uint8_t *out, uint8_t addr, uint8_t func, uint16_t reg, uint16_t n)
{
    out[0] = addr;
    out[1] = func;
    out[2] = (uint8_t)(reg >> 8);
    out[3] = (uint8_t)(reg & 0xFF);
    out[4] = (uint8_t)(n >> 8);
    out[5] = (uint8_t)(n & 0xFF);
    return mb_append_crc(out, 6);
}

int mb_build_write_single(uint8_t *out, uint8_t addr, uint16_t reg, uint16_t val)
{
    out[0] = addr;
    out[1] = MB_FC_WRITE_SINGLE;
    out[2] = (uint8_t)(reg >> 8);
    out[3] = (uint8_t)(reg & 0xFF);
    out[4] = (uint8_t)(val >> 8);
    out[5] = (uint8_t)(val & 0xFF);
    return mb_append_crc(out, 6);
}

int mb_build_write_multi(uint8_t *out, uint8_t addr, uint16_t reg,
                         uint16_t n, const uint16_t *vals)
{
    out[0] = addr;
    out[1] = MB_FC_WRITE_MULTI;
    out[2] = (uint8_t)(reg >> 8);
    out[3] = (uint8_t)(reg & 0xFF);
    out[4] = (uint8_t)(n >> 8);
    out[5] = (uint8_t)(n & 0xFF);
    out[6] = (uint8_t)(n * 2);                   /* 字节数 = 2n */
    for (uint16_t i = 0; i < n; i++) {
        out[7 + 2 * i] = (uint8_t)(vals[i] >> 8);
        out[8 + 2 * i] = (uint8_t)(vals[i] & 0xFF);
    }
    return mb_append_crc(out, 7 + 2 * (int)n);
}

int mb_parse_read_resp(const uint8_t *resp, int len, uint8_t addr, uint8_t func,
                       uint16_t n, uint16_t *out)
{
    if (len < 5)
        return MB_E_FRAME;
    if (resp[0] != addr)
        return MB_E_FRAME;

    /* 异常响应：功能码最高位置 1 + 异常码，固定 5 字节 */
    if (resp[1] & 0x80) {
        if (resp[1] != (uint8_t)(func | 0x80))
            return MB_E_FRAME;
        if (len != 5)
            return MB_E_FRAME;
        if (mb_crc16(resp, (size_t)len) != 0)
            return MB_E_CRC;
        return MB_ERR_EXC(resp[2]);
    }

    if (resp[1] != func)
        return MB_E_FRAME;
    uint8_t bc = resp[2];
    if (bc != (uint8_t)(n * 2))
        return MB_E_FRAME;
    if (len != 3 + bc + 2)
        return MB_E_FRAME;
    if (mb_crc16(resp, (size_t)len) != 0)
        return MB_E_CRC;

    for (uint16_t i = 0; i < n; i++)
        out[i] = (uint16_t)(((uint16_t)resp[3 + 2 * i] << 8) | resp[4 + 2 * i]);
    return (int)n;
}

float mb_regs_to_float32(uint16_t r0, uint16_t r1, mb_word_order_t order)
{
    /* b[0..3] 为 IEEE754 大端内存序 A B C D */
    uint8_t b[4];
    switch (order) {
    case MB_WORD_CDAB:
        b[0] = (uint8_t)(r1 >> 8); b[1] = (uint8_t)(r1 & 0xFF);
        b[2] = (uint8_t)(r0 >> 8); b[3] = (uint8_t)(r0 & 0xFF);
        break;
    case MB_WORD_BADC:
        b[0] = (uint8_t)(r0 & 0xFF); b[1] = (uint8_t)(r0 >> 8);
        b[2] = (uint8_t)(r1 & 0xFF); b[3] = (uint8_t)(r1 >> 8);
        break;
    case MB_WORD_DCBA:
        b[0] = (uint8_t)(r1 & 0xFF); b[1] = (uint8_t)(r1 >> 8);
        b[2] = (uint8_t)(r0 & 0xFF); b[3] = (uint8_t)(r0 >> 8);
        break;
    case MB_WORD_ABCD:
    default:
        b[0] = (uint8_t)(r0 >> 8); b[1] = (uint8_t)(r0 & 0xFF);
        b[2] = (uint8_t)(r1 >> 8); b[3] = (uint8_t)(r1 & 0xFF);
        break;
    }
    /* A B C D 是"大端"字节序 → 先拼成 32 位整数，再按本机浮点格式解释 */
    uint32_t v = ((uint32_t)b[0] << 24) | ((uint32_t)b[1] << 16)
               | ((uint32_t)b[2] << 8)  | (uint32_t)b[3];
    float f;
    memcpy(&f, &v, sizeof f);
    return f;
}

/* ---------------- 串口初始化 ---------------- */

static int mb_baud_const(int baud)
{
    switch (baud) {
    case 9600:   return B9600;
    case 19200:  return B19200;
    case 38400:  return B38400;
    case 57600:  return B57600;
    case 115200: return B115200;
    case 230400: return B230400;
    default:     return -1;
    }
}

static int mb_setup_port(int fd, int baud)
{
    int spd = mb_baud_const(baud);
    if (spd < 0)
        return MB_E_PARAM;

    struct termios tio;
    if (tcgetattr(fd, &tio) != 0)
        return MB_E_IO;

    cfmakeraw(&tio);                             /* 原始模式：8N1、无回显、无行缓冲 */
    tio.c_cflag |= (CLOCAL | CREAD);
    tio.c_cflag &= ~CSIZE;
    tio.c_cflag |= CS8;
    tio.c_cflag &= ~PARENB;                      /* 无校验 */
    tio.c_cflag &= ~CSTOPB;                      /* 1 停止位 */
    tio.c_cc[VMIN]  = 0;
    tio.c_cc[VTIME] = 0;

    if (cfsetispeed(&tio, (speed_t)spd) != 0 || cfsetospeed(&tio, (speed_t)spd) != 0)
        return MB_E_IO;
    if (tcsetattr(fd, TCSANOW, &tio) != 0)
        return MB_E_IO;

    tcflush(fd, TCIOFLUSH);                      /* 上电清残帧 */
    return MB_OK;
}

static void mb_set_rts(int fd, int level)
{
    int bit = TIOCM_RTS;
    if (ioctl(fd, level ? TIOCMBIS : TIOCMBIC, &bit) != 0)
        (void)0;   /* 无 RTS 的适配器(如部分 pty)忽略即可 */
}

/* 尝试让内核接管 RS485 方向控制；失败返回 0，调用方改用 RTS 兜底 */
static int mb_try_kernel_rs485(int fd)
{
#if defined(TIOCSRS485) && defined(SER_RS485_ENABLED)
    struct serial_rs485 rs;
    memset(&rs, 0, sizeof rs);
    rs.flags = SER_RS485_ENABLED | SER_RS485_RTS_ON_SEND;   /* 发送时 RTS 有效，发完自动收回 */
    return (ioctl(fd, TIOCSRS485, &rs) == 0) ? 1 : 0;
#else
    (void)fd;
    return 0;
#endif
}

mb_ctx_t *mb_open(const char *dev, const mb_cfg_t *cfg)
{
    if (!dev)
        return NULL;

    mb_cfg_t def;
    if (!cfg) {
        MB_CFG_DEFAULT_INIT(def);
        cfg = &def;
    }
    if (cfg->slave == 0 || cfg->slave > 247 || cfg->timeout_ms <= 0) {
        errno = EINVAL;
        return NULL;
    }

    int fd = open(dev, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd < 0)
        return NULL;

    if (mb_setup_port(fd, cfg->baud) != MB_OK) {
        close(fd);
        errno = EINVAL;
        return NULL;
    }

    mb_ctx_t *ctx = (mb_ctx_t *)calloc(1, sizeof *ctx);
    if (!ctx) {
        close(fd);
        return NULL;
    }
    ctx->fd         = fd;
    ctx->slave      = cfg->slave;
    ctx->baud       = cfg->baud;
    ctx->timeout_ms = cfg->timeout_ms;
    ctx->retries    = cfg->retries < 0 ? 0 : cfg->retries;
    ctx->rs485      = cfg->rs485;
    ctx->word_order = cfg->word_order;

    if (ctx->rs485)
        ctx->rs485_kernel = mb_try_kernel_rs485(fd);
    return ctx;
}

void mb_close(mb_ctx_t *ctx)
{
    if (!ctx)
        return;
    if (ctx->fd >= 0)
        close(ctx->fd);
    free(ctx);
}

/* ---------------- 收发 ---------------- */

/* RS485 帧间静默：>19200 固定 1.75ms；否则 3.5 * 11bit / baud */
static void mb_t35_delay(const mb_ctx_t *ctx)
{
    long ns;
    if (ctx->baud > 19200)
        ns = 1750000L;
    else
        ns = (long)(3.5 * 11.0 * 1e9 / (double)ctx->baud);

    struct timespec ts;
    ts.tv_sec  = ns / 1000000000L;
    ts.tv_nsec = ns % 1000000000L;
    nanosleep(&ts, NULL);
}

static int mb_tx(mb_ctx_t *ctx, const uint8_t *buf, int len)
{
    if (ctx->rs485 && !ctx->rs485_kernel)
        mb_set_rts(ctx->fd, 1);                  /* 切"发" */

    int sent = 0;
    while (sent < len) {
        ssize_t w = write(ctx->fd, buf + sent, (size_t)(len - sent));
        if (w > 0) {
            sent += (int)w;
        } else if (w < 0 && (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)) {
            struct pollfd pfd = { .fd = ctx->fd, .events = POLLOUT };
            if (poll(&pfd, 1, ctx->timeout_ms) <= 0)
                break;
        } else {
            break;
        }
    }
    int ok = (sent == len);
    if (ok)
        tcdrain(ctx->fd);                        /* ★ 等最后一个字节真正移出，再切收 */

    if (ctx->rs485 && !ctx->rs485_kernel)
        mb_set_rts(ctx->fd, 0);                  /* 切"收" */
    return ok ? MB_OK : MB_E_IO;
}

/* 由已收字节推断整帧长度；未知返回 0 */
static int mb_expected_len(const uint8_t *b, int len)
{
    if (len < 2)
        return 0;
    if (b[1] & 0x80)
        return 5;                                /* 异常响应固定 5 字节 */
    switch (b[1]) {
    case MB_FC_READ_HOLDING:
    case MB_FC_READ_INPUT:
        return (len < 3) ? 0 : (3 + b[2] + 2);
    case MB_FC_WRITE_SINGLE:
    case MB_FC_WRITE_MULTI:
        return 8;
    default:
        return 0;
    }
}

/* 收一帧；返回帧长(含 CRC)，负值为错误码 */
static int mb_rx(mb_ctx_t *ctx, uint8_t *buf, int cap)
{
    struct pollfd pfd = { .fd = ctx->fd, .events = POLLIN };
    int rc = poll(&pfd, 1, ctx->timeout_ms);
    if (rc == 0)
        return MB_E_TIMEOUT;
    if (rc < 0)
        return (errno == EINTR) ? MB_E_TIMEOUT : MB_E_IO;

    int len = 0;
    for (;;) {
        ssize_t n = read(ctx->fd, buf + len, (size_t)(cap - len));
        if (n > 0) {
            len += (int)n;
        } else if (n < 0 && errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR) {
            return MB_E_IO;
        }
        int need = mb_expected_len(buf, len);
        if (need > 0 && len >= need)
            return len;
        if (len >= cap)
            return MB_E_FRAME;

        struct pollfd p2 = { .fd = ctx->fd, .events = POLLIN };
        if (poll(&p2, 1, MB_INTERCHAR_MS) <= 0)
            return (len == 0) ? MB_E_TIMEOUT : MB_E_FRAME;
    }
}

static void mb_note_ok(mb_ctx_t *ctx)
{
    ctx->stat_ok++;
    ctx->consec_fail = 0;
}

static void mb_note_fail(mb_ctx_t *ctx)
{
    ctx->stat_err++;
    if (ctx->consec_fail < 1000000)
        ctx->consec_fail++;
}

/* ---------------- 事务 ---------------- */

int mb_read_regs(mb_ctx_t *ctx, uint8_t func, uint16_t reg, uint16_t n, uint16_t *out)
{
    if (!ctx || !out)
        return MB_E_PARAM;
    if ((func != MB_FC_READ_HOLDING && func != MB_FC_READ_INPUT) || n == 0 || n > 125)
        return MB_E_PARAM;

    uint8_t req[16], resp[MB_MAX_ADU];
    int reqlen = mb_build_read(req, ctx->slave, func, reg, n);

    int last = MB_E_TIMEOUT;
    for (int attempt = 0; attempt <= ctx->retries; attempt++) {
        mb_t35_delay(ctx);
        if (mb_tx(ctx, req, reqlen) != MB_OK) { mb_note_fail(ctx); last = MB_E_IO; continue; }

        int rl = mb_rx(ctx, resp, sizeof resp);
        if (rl < 0) { mb_note_fail(ctx); last = rl; continue; }

        int r = mb_parse_read_resp(resp, rl, ctx->slave, func, n, out);
        if (MB_IS_EXC(r)) {                      /* 从站明确报错：不该重试(学习册 §7/§11-6) */
            mb_note_fail(ctx);
            return r;
        }
        if (r < 0) { mb_note_fail(ctx); last = r; continue; }   /* CRC/帧错 → 重试 */

        mb_note_ok(ctx);
        return r;
    }
    return last;
}

/* 校验 0x06 的"原样回显" / 0x10 的"起始地址+数量"回显 */
static int mb_parse_write_resp(const uint8_t *resp, int len, uint8_t addr,
                               uint8_t func, uint16_t reg, uint16_t n)
{
    if (len != 8 || resp[0] != addr)
        return MB_E_FRAME;
    if (resp[1] & 0x80) {
        if (resp[1] != (uint8_t)(func | 0x80))
            return MB_E_FRAME;
        /* 异常响应 5 字节，这里长度已是 8，直接判 CRC 再报异常 */
        return MB_E_FRAME;
    }
    if (resp[1] != func)
        return MB_E_FRAME;
    if (mb_crc16(resp, (size_t)len) != 0)
        return MB_E_CRC;
    uint16_t r = (uint16_t)(((uint16_t)resp[2] << 8) | resp[3]);
    if (r != reg)
        return MB_E_FRAME;
    if (func == MB_FC_WRITE_MULTI) {
        uint16_t cnt = (uint16_t)(((uint16_t)resp[4] << 8) | resp[5]);
        if (cnt != n)
            return MB_E_FRAME;
    }
    return MB_OK;
}

static int mb_write_common(mb_ctx_t *ctx, uint8_t func, uint16_t reg,
                           uint16_t n, const uint16_t *vals)
{
    uint8_t req[MB_MAX_ADU], resp[MB_MAX_ADU];
    int reqlen = (func == MB_FC_WRITE_SINGLE)
                     ? mb_build_write_single(req, ctx->slave, reg, vals[0])
                     : mb_build_write_multi(req, ctx->slave, reg, n, vals);

    int last = MB_E_TIMEOUT;
    for (int attempt = 0; attempt <= ctx->retries; attempt++) {
        mb_t35_delay(ctx);
        if (mb_tx(ctx, req, reqlen) != MB_OK) { mb_note_fail(ctx); last = MB_E_IO; continue; }

        int rl = mb_rx(ctx, resp, sizeof resp);
        if (rl < 0) { mb_note_fail(ctx); last = rl; continue; }

        /* 异常响应长度 5，单独解析 */
        if (rl == 5 && (resp[1] & 0x80)) {
            if (resp[0] == ctx->slave && resp[1] == (uint8_t)(func | 0x80)
                && mb_crc16(resp, 5) == 0) {
                mb_note_fail(ctx);
                return MB_ERR_EXC(resp[2]);
            }
            mb_note_fail(ctx); last = MB_E_FRAME; continue;
        }

        int r = mb_parse_write_resp(resp, rl, ctx->slave, func, reg, n);
        if (r < 0) { mb_note_fail(ctx); last = r; continue; }

        mb_note_ok(ctx);
        return (func == MB_FC_WRITE_SINGLE) ? 1 : (int)n;
    }
    return last;
}

int mb_write_reg(mb_ctx_t *ctx, uint16_t reg, uint16_t val)
{
    if (!ctx)
        return MB_E_PARAM;
    return mb_write_common(ctx, MB_FC_WRITE_SINGLE, reg, 1, &val);
}

int mb_write_regs(mb_ctx_t *ctx, uint16_t reg, uint16_t n, const uint16_t *vals)
{
    if (!ctx || !vals || n == 0 || n > 123)
        return MB_E_PARAM;
    return mb_write_common(ctx, MB_FC_WRITE_MULTI, reg, n, vals);
}

int mb_read_float32(mb_ctx_t *ctx, uint8_t func, uint16_t reg, float *out)
{
    if (!ctx || !out)
        return MB_E_PARAM;
    uint16_t r[2];
    int n = mb_read_regs(ctx, func, reg, 2, r);
    if (n < 0)
        return n;
    *out = mb_regs_to_float32(r[0], r[1], ctx->word_order);
    return 0;
}

int mb_is_offline(const mb_ctx_t *ctx)
{
    return ctx && ctx->consec_fail >= MB_OFFLINE_THRESHOLD;
}

uint64_t mb_stat_ok(const mb_ctx_t *ctx)  { return ctx ? ctx->stat_ok : 0; }
uint64_t mb_stat_err(const mb_ctx_t *ctx) { return ctx ? ctx->stat_err : 0; }
