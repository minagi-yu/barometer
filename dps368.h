#ifndef DPS368_H
#define DPS368_H

#include <stdint.h>

#define DPS368_ADDR 0x77

#define DPS368_PSR_B2 0x00
#define DPS368_PSR_B1 0x01
#define DPS368_PSR_B0 0x02
#define DPS368_TMP_B2 0x03
#define DPS368_TMP_B1 0x04
#define DPS368_TMP_B0 0x05
#define DPS368_PRS_CFG 0x06
#define DPS368_TMP_CFG 0x07
#define DPS368_MEAS_CFG 0x08
#define DPS368_CFG_REG 0x09
#define DPS368_INT_STS 0x0A
#define DPS368_FIFO_STS 0x0B
#define DPS368_RESET 0x0C
#define DPS368_PROD_ID 0x0D
#define DPS368_COEF 0x10
#define DPS368_COEF_SRCE 0x28

enum dps368_opmode {
    DPS368_OPMODE_IDOL,
    DPS368_PRS_MEAS,
    DPS368_TMP_MEAS,
    DPS368_CONT_PRS_MEAS = 5,
    DPS368_CONT_TMP_MEAS,
    DPS368_CONT_PRS_TMP_MEAS
};

enum dps368_meas_rate {
    DPS368_MEAS_RATE_1,
    DPS368_MEAS_RATE_2 = 16,
    DPS368_MEAS_RATE_4 = 32,
    DPS368_MEAS_RATE_8 = 48,
    DPS368_MEAS_RATE_16 = 64,
    DPS368_MEAS_RATE_32 = 80,
    DPS368_MEAS_RATE_64 = 96,
    DPS368_MEAS_RATE_128 = 112
};

enum dps368_samp_rate {
    DPS368_SAMP_RATE_1,
    DPS368_SAMP_RATE_2,
    DPS368_SAMP_RATE_4,
    DPS368_SAMP_RATE_8,
    DPS368_SAMP_RATE_16,
    DPS368_SAMP_RATE_32,
    DPS368_SAMP_RATE_64,
    DPS368_SAMP_RATE_128
};

#define DPS368_INT_FIFO 0x40
#define DPS368_INT_TMP 0x20
#define DPS368_INT_PRS 0x10

void dps368_init(void);
void dps368_clear_intflgs(void);
void dps368_set_opmode(enum dps368_opmode mode);
void dps368_config_tmp(enum dps368_meas_rate mr, enum dps368_samp_rate sr);
void dps368_config_prs(enum dps368_meas_rate mr, enum dps368_samp_rate sr);
void dps368_config_int(uint8_t int_source);
void dps368_get_result(int32_t *tmp, int32_t *prs);

#endif
