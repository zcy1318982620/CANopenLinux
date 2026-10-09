/*
 * agv_modbus 单元测试（零框架，见 tests/agv_test.h）。
 * 只链接 agv_modbus.c，不碰串口/CAN/RT——验证纯函数：
 *   CRC16 已知向量、组帧字节序、读响应解析、异常响应解析、float32 四种字序。
 * 运行：make test 或 ctest（对应目标 test_agv_modbus）。
 */

#include "agv_test.h"
#include "agv_modbus.h"

#include <stdint.h>

/* 给 buf[0..len-1] 追加 CRC(低字节先发)，返回新长度 */
static int put_crc(uint8_t *buf, int len)
{
    uint16_t c = mb_crc16(buf, (size_t)len);
    buf[len]     = (uint8_t)(c & 0xFF);
    buf[len + 1] = (uint8_t)(c >> 8);
    return len + 2;
}

/* ---- CRC16-Modbus ---- */
static void test_crc(void)
{
    TEST_BEGIN("crc16-modbus");

    /* 标准校验串："123456789" → 0x4B37 */
    CHECK_EQ_I64(mb_crc16((const uint8_t *)"123456789", 9), 0x4B37);

    /* 经典请求 01 03 00 00 00 02 → CRC 0x0BC4（发送时 C4 0B） */
    const uint8_t req6[6] = { 0x01, 0x03, 0x00, 0x00, 0x00, 0x02 };
    CHECK_EQ_I64(mb_crc16(req6, 6), 0x0BC4);

    /* 追加 CRC 后整帧自校验为 0（协议特性） */
    uint8_t full[8];
    for (int i = 0; i < 6; i++) full[i] = req6[i];
    put_crc(full, 6);
    CHECK_EQ_I64(mb_crc16(full, 8), 0);
}

/* ---- 组帧 ---- */
static void test_build(void)
{
    TEST_BEGIN("build-frame");
    uint8_t f[64];

    /* 读请求：01 03 00 00 00 02 C4 0B */
    int n = mb_build_read(f, 0x01, MB_FC_READ_HOLDING, 0x0000, 0x0002);
    CHECK_EQ_I64(n, 8);
    const uint8_t exp[8] = { 0x01, 0x03, 0x00, 0x00, 0x00, 0x02, 0xC4, 0x0B };
    CHECK(memcmp(f, exp, 8) == 0);
    CHECK_EQ_I64(mb_crc16(f, 8), 0);

    /* 功能码 0x04 也要有位（读输入寄存器） */
    CHECK_EQ_I64(mb_build_read(f, 0x11, MB_FC_READ_INPUT, 0x0006, 0x0003), 8);
    CHECK_EQ_I64(f[0], 0x11);
    CHECK_EQ_I64(f[1], 0x04);
    CHECK_EQ_I64((f[2] << 8) | f[3], 0x0006);
    CHECK_EQ_I64((f[4] << 8) | f[5], 0x0003);
    CHECK_EQ_I64(mb_crc16(f, 8), 0);

    /* 写单个：01 06 01 01 00 64 + CRC */
    n = mb_build_write_single(f, 0x01, 0x0101, 0x0064);
    CHECK_EQ_I64(n, 8);
    CHECK_EQ_I64(f[1], 0x06);
    CHECK_EQ_I64((f[4] << 8) | f[5], 0x0064);
    CHECK_EQ_I64(mb_crc16(f, 8), 0);

    /* 写多个：01 10 00 00 00 02 04 xx xx xx xx + CRC（帧长 9+2n=13） */
    const uint16_t vals[2] = { 0x3FC0, 0x0000 };
    n = mb_build_write_multi(f, 0x01, 0x0000, 2, vals);
    CHECK_EQ_I64(n, 13);
    CHECK_EQ_I64(f[1], 0x10);
    CHECK_EQ_I64(f[6], 0x04);          /* 字节数 = 2n */
    CHECK_EQ_I64(f[7], 0x3F);
    CHECK_EQ_I64(f[8], 0xC0);
    CHECK_EQ_I64(mb_crc16(f, 13), 0);
}

/* ---- 读响应解析 ---- */
static void test_parse_read(void)
{
    TEST_BEGIN("parse-read-resp");
    uint8_t r[32];
    uint16_t out[4];

    /* 正常：01 04 08 + 4 个寄存器 + CRC */
    r[0] = 0x01; r[1] = 0x04; r[2] = 0x08;
    r[3] = 0x3F; r[4] = 0xC0;   /* 0x3FC0 */
    r[5] = 0x00; r[6] = 0x00;
    r[7] = 0x12; r[8] = 0x34;
    r[9] = 0xAB; r[10] = 0xCD;
    int len = put_crc(r, 11);
    CHECK_EQ_I64(len, 13);
    CHECK_EQ_I64(mb_parse_read_resp(r, len, 0x01, MB_FC_READ_INPUT, 4, out), 4);
    CHECK_EQ_I64(out[0], 0x3FC0);
    CHECK_EQ_I64(out[1], 0x0000);
    CHECK_EQ_I64(out[2], 0x1234);
    CHECK_EQ_I64(out[3], 0xABCD);

    /* 从站地址不匹配 → 帧错误 */
    CHECK_EQ_I64(mb_parse_read_resp(r, len, 0x02, MB_FC_READ_INPUT, 4, out), MB_E_FRAME);

    /* 功能码不匹配 → 帧错误 */
    CHECK_EQ_I64(mb_parse_read_resp(r, len, 0x01, MB_FC_READ_HOLDING, 4, out), MB_E_FRAME);

    /* 字节数与请求不符 → 帧错误 */
    CHECK_EQ_I64(mb_parse_read_resp(r, len, 0x01, MB_FC_READ_INPUT, 3, out), MB_E_FRAME);

    /* CRC 被破坏 → CRC 错误 */
    r[len - 1] ^= 0xFF;
    CHECK_EQ_I64(mb_parse_read_resp(r, len, 0x01, MB_FC_READ_INPUT, 4, out), MB_E_CRC);
}

/* ---- 异常响应 ---- */
static void test_parse_exception(void)
{
    TEST_BEGIN("parse-exception");
    uint8_t r[8];
    uint16_t out[2];

    r[0] = 0x01; r[1] = 0x84; r[2] = MB_EXC_ILLEGAL_ADDR;   /* 0x04|0x80 */
    int len = put_crc(r, 3);
    CHECK_EQ_I64(len, 5);

    int rc = mb_parse_read_resp(r, len, 0x01, MB_FC_READ_INPUT, 2, out);
    CHECK(MB_IS_EXC(rc));
    CHECK_EQ_I64(MB_EXC_CODE(rc), MB_EXC_ILLEGAL_ADDR);
    CHECK_EQ_I64(rc, MB_ERR_EXC(MB_EXC_ILLEGAL_ADDR));

    /* 异常响应 CRC 被破坏 → CRC 错误，而不是误报异常 */
    r[4] ^= 0xFF;
    CHECK_EQ_I64(mb_parse_read_resp(r, len, 0x01, MB_FC_READ_INPUT, 2, out), MB_E_CRC);
}

/* ---- float32 四种字序 ---- */
static void test_float_order(void)
{
    TEST_BEGIN("float32-word-order");

    /* 1.5f = 0x3FC00000，内存序 A B C D = 3F C0 00 00 */
    CHECK_EQ_DBL(mb_regs_to_float32(0x3FC0, 0x0000, MB_WORD_ABCD), 1.5f, 1e-6);
    CHECK_EQ_DBL(mb_regs_to_float32(0x0000, 0x3FC0, MB_WORD_CDAB), 1.5f, 1e-6);
    CHECK_EQ_DBL(mb_regs_to_float32(0xC03F, 0x0000, MB_WORD_BADC), 1.5f, 1e-6);
    CHECK_EQ_DBL(mb_regs_to_float32(0x0000, 0xC03F, MB_WORD_DCBA), 1.5f, 1e-6);

    /* -2.5f = 0xC0200000 → A B C D = C0 20 00 00 */
    CHECK_EQ_DBL(mb_regs_to_float32(0xC020, 0x0000, MB_WORD_ABCD), -2.5f, 1e-6);
    CHECK_EQ_DBL(mb_regs_to_float32(0x0000, 0xC020, MB_WORD_CDAB), -2.5f, 1e-6);
    CHECK_EQ_DBL(mb_regs_to_float32(0x20C0, 0x0000, MB_WORD_BADC), -2.5f, 1e-6);
    CHECK_EQ_DBL(mb_regs_to_float32(0x0000, 0x20C0, MB_WORD_DCBA), -2.5f, 1e-6);

    /* 3.14159f = 0x40490FD0 → A B C D = 40 49 0F D0 */
    CHECK_EQ_DBL(mb_regs_to_float32(0x4049, 0x0FD0, MB_WORD_ABCD), 3.14159f, 1e-5);
}

int main(void)
{
    test_crc();
    test_build();
    test_parse_read();
    test_parse_exception();
    test_float_order();
    return TEST_SUMMARY();
}
