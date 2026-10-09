/*
 * Modbus RTU 主站（RS485 半双工）——用于接入 Modbus 接口的 IMU。
 *
 * 设计与逐字节讲解见 docs/MODBUS_LEARNING_MAP_print.html（§5 帧/CRC、§7 异常码、
 * §8 字节序、§10 落地）。本模块只做"主站"这一层：组帧/解帧、CRC16、
 * 超时重试、异常识别、float32 跨寄存器字序还原、RS485 方向控制。
 *
 * 分层原则（与 agv_queue/agv_pool 一致）：本模块**不依赖 agv_log**，
 * 所有错误以返回码表达，调用方自行分级打印；这样 test_agv_modbus 只链接
 * agv_modbus.c 即可对纯函数（CRC/组帧/解帧/字序）做单测，不需要串口硬件。
 *
 * 返回码约定：
 *   >= 0            成功（读/写成功的寄存器个数）
 *   -1..-15         本地错误码（MB_E_xxx）
 *   MB_ERR_EXC(code) 从站异常响应（-257 .. -511），用 MB_IS_EXC/MB_EXC_CODE 解析
 */

#ifndef AGV_MODBUS_H
#define AGV_MODBUS_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ---- 功能码 ---- */
#define MB_FC_READ_HOLDING   0x03   /* 读保持寄存器(4x，RW) */
#define MB_FC_READ_INPUT     0x04   /* 读输入寄存器(3x，RO) */
#define MB_FC_WRITE_SINGLE   0x06   /* 写单个保持寄存器 */
#define MB_FC_WRITE_MULTI    0x10   /* 写多个保持寄存器 */

/* ---- 本地错误码 ---- */
#define MB_OK              0
#define MB_E_PARAM        (-1)      /* 入参非法(地址/数量/功能码) */
#define MB_E_IO           (-2)      /* 串口读写/配置失败 */
#define MB_E_TIMEOUT      (-3)      /* 等响应超时 */
#define MB_E_CRC          (-4)      /* CRC 校验失败 */
#define MB_E_FRAME        (-5)      /* 帧结构非法(长度/地址/功能码不匹配) */

/* ---- 从站异常响应：ADU 中 功能码|0x80 + 异常码 ---- */
#define MB_ERR_EXC(code)  (-(int)(0x100 | ((code) & 0xFF)))
#define MB_IS_EXC(r)      ((r) <= -257 && (r) >= -511)
#define MB_EXC_CODE(r)    ((-(r)) & 0xFF)
/* 常用异常码 */
#define MB_EXC_ILLEGAL_FUNC    0x01 /* 非法功能码 */
#define MB_EXC_ILLEGAL_ADDR    0x02 /* 非法数据地址 */
#define MB_EXC_ILLEGAL_VALUE   0x03 /* 非法数据值 */
#define MB_EXC_SLAVE_FAILURE   0x04 /* 从站设备故障 */

/* 连续失败达到该次数即判定从站离线（供 agv_is_offline 使用） */
#define MB_OFFLINE_THRESHOLD   3

/* float32 跨两个 16 位寄存器的四种字节序（见学习册 §8）。
 * 记 r0=低地址寄存器、r1=高地址寄存器，最终还原成内存序 [A,B,C,D]（大端 IEEE754）。 */
typedef enum {
    MB_WORD_ABCD = 0,   /* 大端：r0=AB, r1=CD（最常见） */
    MB_WORD_CDAB = 1,   /* 字交换：r0=CD, r1=AB */
    MB_WORD_BADC = 2,   /* 字节交换：r0=BA, r1=DC */
    MB_WORD_DCBA = 3    /* 全小端：r0=DC, r1=BA */
} mb_word_order_t;

/* 主站配置 */
typedef struct {
    int             baud;        /* 波特率：9600/19200/38400/57600/115200/230400 */
    uint8_t         slave;       /* 从站地址(1..247) */
    int             timeout_ms;  /* 单次等响应超时(ms) */
    int             retries;     /* 失败重试次数(不含首次)。异常响应不重试 */
    int             rs485;       /* 1=启用 RS485 方向控制(优先内核 ioctl，兜底 RTS) */
    mb_word_order_t word_order;  /* float32 字序，接真机后按 §8 实测校准 */
} mb_cfg_t;

/* 默认配置：115200 8N1、从站 1、300ms 超时、重试 2 次、ABCD 字序 */
#define MB_CFG_DEFAULT_INIT(cfg)                        \
    do {                                                \
        (cfg).baud       = 115200;                      \
        (cfg).slave      = 1;                           \
        (cfg).timeout_ms = 300;                         \
        (cfg).retries    = 2;                           \
        (cfg).rs485      = 1;                           \
        (cfg).word_order = MB_WORD_ABCD;                \
    } while (0)

/* 不透明句柄 */
typedef struct mb_ctx mb_ctx_t;

/* ================= 串口 + 事务层 ================= */

/*
 * 打开串口并初始化主站。dev 为设备路径(如 /dev/ttyS1 或无硬件时的 /dev/pts/N)。
 * cfg 为 NULL 时使用默认配置。
 * @return 句柄；失败返回 NULL(并置 errno)。
 */
mb_ctx_t *mb_open(const char *dev, const mb_cfg_t *cfg);

/* 关闭串口并释放句柄。 */
void      mb_close(mb_ctx_t *ctx);

/*
 * 读寄存器。func 取 MB_FC_READ_HOLDING(0x03) 或 MB_FC_READ_INPUT(0x04)。
 * n 为寄存器个数(1..125)，结果按大端解到 out[0..n-1]。
 * @return n(成功)；负值为错误码(超时/CRC/帧错/异常)。
 */
int mb_read_regs(mb_ctx_t *ctx, uint8_t func, uint16_t reg, uint16_t n, uint16_t *out);

/* 写单个保持寄存器(0x06)。@return 1；负值为错误码。 */
int mb_write_reg(mb_ctx_t *ctx, uint16_t reg, uint16_t val);

/* 写多个保持寄存器(0x10)。@return n；负值为错误码。 */
int mb_write_regs(mb_ctx_t *ctx, uint16_t reg, uint16_t n, const uint16_t *vals);

/*
 * 便捷：读 2 个寄存器并按配置字序还原成 float32。
 * @return 0 成功；负值为错误码。
 */
int mb_read_float32(mb_ctx_t *ctx, uint8_t func, uint16_t reg, float *out);

/* 从站是否离线(连续失败 >= MB_OFFLINE_THRESHOLD)。 */
int mb_is_offline(const mb_ctx_t *ctx);

/* 统计计数(调试/观测用)。 */
uint64_t mb_stat_ok(const mb_ctx_t *ctx);
uint64_t mb_stat_err(const mb_ctx_t *ctx);

/* ================= 纯函数(可单测，不碰硬件) ================= */

/* CRC16-Modbus：多项式 0xA001、初值 0xFFFF。发送时低字节在前。 */
uint16_t mb_crc16(const uint8_t *buf, size_t len);

/* 组"读寄存器"请求帧(含 CRC)，返回帧长(恒为 8)。 */
int mb_build_read(uint8_t *out, uint8_t addr, uint8_t func, uint16_t reg, uint16_t n);

/* 组"写单个寄存器"请求帧，返回帧长(恒为 8)。 */
int mb_build_write_single(uint8_t *out, uint8_t addr, uint16_t reg, uint16_t val);

/* 组"写多个寄存器"请求帧，返回帧长(9 + 2*n)。 */
int mb_build_write_multi(uint8_t *out, uint8_t addr, uint16_t reg,
                         uint16_t n, const uint16_t *vals);

/*
 * 解析"读寄存器"响应帧。成功把数据写入 out[0..n-1]，返回 n；
 * 从站异常返回 MB_ERR_EXC(code)；其余负值为帧/CRC 错误。
 */
int mb_parse_read_resp(const uint8_t *resp, int len, uint8_t addr, uint8_t func,
                       uint16_t n, uint16_t *out);

/*
 * 把两个寄存器还原成 float32（按 order 组合字节后按大端解释）。见学习册 §8。
 */
float mb_regs_to_float32(uint16_t r0, uint16_t r1, mb_word_order_t order);

#ifdef __cplusplus
}
#endif

#endif /* AGV_MODBUS_H */
