#include "dps368.h"
#include "i2c.h"

#define DPS368_COEF_SIZE 18
#define DPS368_RES_SIZE 3

static uint8_t source;
// static int32_t c0, c1, c00, c10, c01, c11, c20, c21, c30;
static int16_t c0, c1, c01, c11, c20, c21, c30;
static int32_t c00, c10;
static enum dps368_samp_rate sr_tmp, sr_prs;

static const float scale_factor[] = {
    524288, 1572864, 3670016, 7864320,
    253952, 516096, 1040384, 2088960
};

static void dps368_write(uint8_t addr, uint8_t data)
{
    uint8_t d[2];
    d[0] = addr;
    d[1] = data;
    i2c_write(DPS368_ADDR, &d, 2);
    i2c_stop();
}

static uint8_t dps368_read(uint8_t addr)
{
    uint8_t r;
    i2c_write(DPS368_ADDR, &addr, 1);
    i2c_read(DPS368_ADDR, &r, 1);
    i2c_stop();
    return r;
}

static void dps368_read_bytes(uint8_t addr, void *buff, uint16_t len)
{
    i2c_write(DPS368_ADDR, &addr, 1);
    i2c_read(DPS368_ADDR, buff, len);
    i2c_stop();
}

static void dps368_read_tmp_sens(void)
{
    source = dps368_read(DPS368_COEF_SRCE);
    source &= 0x80;
}

// static void dps368_get_comp(int32_t *val, uint8_t len)
// {
//     if (*val & (UINT32_C(1) << (len - 1))) {
//         *val -= UINT32_C(1) << len;
//     }
// }

__attribute__((naked)) static int32_t dps368_get_comp24(uint32_t val)
{
    asm(
        "mov    r0, r24\n"
        "add    r0, r0\n"
        "sbc    r25, r25\n"
        "ret\n");
}
__attribute__((naked)) static int32_t dps368_get_comp20(uint32_t val)
{
    asm(
        "sbrc   r24, 3\n"
        "ori    r24, 0xf0\n"
        "clr    r25\n"
        "sbrc   r24, 3\n"
        "ser    r25\n"
        "ret\n");
}
__attribute__((naked)) static int16_t dps368_get_comp12(uint16_t val)
{
    asm(
        "sbrc   r25, 3\n"
        "ori    r25, 0xf0\n"
        "ret\n");
}

static void dps368_read_coefs(void)
{
    uint8_t coefs[DPS368_COEF_SIZE];

    dps368_read_bytes(DPS368_COEF, coefs, DPS368_COEF_SIZE);

    // c0 = ((uint32_t)coefs[0] << 4) | (((uint32_t)coefs[1] >> 4) & 0x0f);
    // dps368_get_comp(&c0, 12);
    c0 = dps368_get_comp12(((uint16_t)coefs[0] << 4) | (((uint16_t)coefs[1] >> 4) & 0x0f));

    // c1 = (((uint32_t)coefs[1] & 0x0f) << 8) | (uint32_t)coefs[2];
    // dps368_get_comp(&c1, 12);
    c1 = dps368_get_comp12((((uint16_t)coefs[1] & 0x0f) << 8) | (uint16_t)coefs[2]);

    // c00 = ((uint32_t)coefs[3] << 12) | ((uint32_t)coefs[4] << 4) | (((uint32_t)coefs[5] >> 4) & 0x0f);
    // dps368_get_comp(&c00, 20);
    c00 = dps368_get_comp20(((uint32_t)coefs[3] << 12) | ((uint32_t)coefs[4] << 4) | (((uint32_t)coefs[5] >> 4) & 0x0f));

    // c10 = (((uint32_t)coefs[5] & 0x0f) << 16) | ((uint32_t)coefs[6] << 8) | (uint32_t)coefs[7];
    // dps368_get_comp(&c10, 20);
    c10 = dps368_get_comp20((((uint32_t)coefs[5] & 0x0f) << 16) | ((uint32_t)coefs[6] << 8) | (uint32_t)coefs[7]);

    // c01 = ((uint32_t)coefs[8] << 8) | (uint32_t)coefs[9];
    // dps368_get_comp(&c01, 16);
    c01 = ((uint16_t)coefs[8] << 8) | (uint16_t)coefs[9];

    // c11 = ((uint32_t)coefs[10] << 8) | (uint32_t)coefs[11];
    // dps368_get_comp(&c11, 16);
    c11 = ((uint16_t)coefs[10] << 8) | (uint16_t)coefs[11];

    // c20 = ((uint32_t)coefs[12] << 8) | (uint32_t)coefs[13];
    // dps368_get_comp(&c20, 16);
    c20 = ((uint16_t)coefs[12] << 8) | (uint16_t)coefs[13];

    // c21 = ((uint32_t)coefs[14] << 8) | (uint32_t)coefs[15];
    // dps368_get_comp(&c21, 16);
    c21 = ((uint16_t)coefs[14] << 8) | (uint16_t)coefs[15];

    // c30 = ((uint32_t)coefs[16] << 8) | (uint32_t)coefs[17];
    // dps368_get_comp(&c30, 16);
    c30 = ((uint16_t)coefs[16] << 8) | (uint16_t)coefs[17];
}

void dps368_set_opmode(enum dps368_opmode mode)
{
    dps368_write(DPS368_MEAS_CFG, mode);
}

void dps368_config_tmp(enum dps368_meas_rate mr, enum dps368_samp_rate sr)
{
    dps368_write(DPS368_TMP_CFG, source | mr | sr);
    sr_tmp = sr;
}

void dps368_config_prs(enum dps368_meas_rate mr, enum dps368_samp_rate sr)
{
    dps368_write(DPS368_PRS_CFG, mr | sr);
    sr_prs = sr;
}

void dps368_config_int(uint8_t int_source)
{
    uint8_t reg = 0;
    if (sr_tmp >= DPS368_SAMP_RATE_16)
        reg |= (1 << 3);
    if (sr_prs >= DPS368_SAMP_RATE_16)
        reg |= (1 << 2);
    reg |= int_source;
    dps368_write(DPS368_CFG_REG, reg);
}

void dps368_init(void)
{
    dps368_read_tmp_sens();
    dps368_read_coefs();
    dps368_set_opmode(DPS368_OPMODE_IDOL);
}

void dps368_clear_intflgs(void)
{
    dps368_read(DPS368_INT_STS);
}

void dps368_get_result(int32_t *tmp, int32_t *prs)
{
    uint8_t res[DPS368_RES_SIZE * 2];
    int32_t t_raw, p_raw;
    float t_raw_sc, p_raw_sc;

    dps368_read_bytes(0x00, res, DPS368_RES_SIZE * 2);

    // p_raw = ((uint32_t)res[0] << 16) | ((uint32_t)res[1] << 8) | ((uint32_t)res[2]);
    // dps368_get_comp(p_raw, 24);
    // t_raw = ((uint32_t)res[3] << 16) | ((uint32_t)res[4] << 8) | ((uint32_t)res[5]);
    // dps368_get_comp(t_raw, 24);
    p_raw = dps368_get_comp24(((uint32_t)res[0] << 16) | ((uint32_t)res[1] << 8) | ((uint32_t)res[2]));
    t_raw = dps368_get_comp24(((uint32_t)res[3] << 16) | ((uint32_t)res[4] << 8) | ((uint32_t)res[5]));

    t_raw_sc = t_raw / scale_factor[sr_tmp];
    *tmp = (c0 * 0.5f + c1 * t_raw_sc) * 100;

    p_raw_sc = p_raw / scale_factor[sr_prs];
    *prs = c00 + p_raw_sc * (c10 + p_raw_sc * (c20 + p_raw_sc * c30)) + t_raw_sc * c01 + t_raw_sc * p_raw_sc * (c11 + p_raw_sc * c21);
}
