#include "i2c.h"

#define I2C_CLK_PER F_CPU
#define I2C_FSCL I2C_SM

#define I2C_DIR_WRITE 0
#define I2C_DIR_READ 1

void i2c_init(void)
{
    TWI0.MBAUD = (I2C_CLK_PER / I2C_FSCL - 10) / 2;

    TWI0.MCTRLA = TWI_ENABLE_bm;

    TWI0.MSTATUS = TWI_BUSSTATE_IDLE_gc;
}

static void i2c_wait()
{
    while (!(TWI0.MSTATUS & (TWI_WIF_bm | TWI_RIF_bm)))
        ;
    // TWI0.MSTATUS |= (TWI_RIF_bm | TWI_WIF_bm);
}

static void i2c_reset()
{
    TWI0.MCTRLB |= TWI_FLUSH_bm;
    TWI0.MSTATUS = TWI_BUSSTATE_IDLE_gc;
}

static uint_fast8_t i2c_transmit_addr(uint8_t addr, uint_fast8_t dir)
{
    do {
        TWI0.MADDR = (addr << 1) | dir;

        i2c_wait();

        if (TWI0.MSTATUS & (TWI_ARBLOST_bm | TWI_BUSERR_bm)) {
            i2c_reset();
            return 1;
        }
    } while (TWI0.MSTATUS & TWI_RXACK_bm); // NACK: rewrite address

    return 0;
}

uint_fast8_t i2c_write(uint8_t addr, void *data, uint16_t len)
{
    uint8_t *dp = data;

    if (len == 0)
        return 1;

    // i2c_reset();
    // TWI0.MSTATUS |= (TWI_RIF_bm | TWI_WIF_bm);

    if (i2c_transmit_addr(addr, I2C_DIR_WRITE))
        return 1;

    // ACK: write data
    do {
        TWI0.MDATA = *dp++;

        i2c_wait();

        if (TWI0.MSTATUS & (TWI_ARBLOST_bm | TWI_BUSERR_bm)) {
            i2c_reset();
            return 1;
        }

        if (TWI0.MSTATUS & TWI_RXACK_bm)
            break;
    } while (--len);

    return 0;
}

uint_fast8_t i2c_read(uint8_t addr, void *data, uint16_t len)
{
    uint8_t *dp = data;

    if (len == 0)
        return 1;

    // i2c_reset();
    // TWI0.MSTATUS |= (TWI_RIF_bm | TWI_WIF_bm);

    if (i2c_transmit_addr(addr, I2C_DIR_READ))
        return 1;

    // ACK: read data
    // do {
    //     if (len == 1)
    //         TWI0.MCTRLB = TWI_ACKACT_NACK_gc;
    //     else
    //         TWI0.MCTRLB = TWI_ACKACT_ACK_gc;

    //     *dp++ = TWI0.MDATA;
    //     len--;

    //     if (len == 0)
    //         break;

    //     TWI0.MCTRLB |= TWI_MCMD_RECVTRANS_gc;

    //     i2c_wait();

    //     if (TWI0.MSTATUS & (TWI_ARBLOST_bm | TWI_BUSERR_bm)) {
    //         i2c_reset();
    //         return 1;
    //     }
    // } while (1);
    while (len--) {
        i2c_wait();
        *dp++ = TWI0.MDATA;
        if (len == 0) {
            TWI0.MCTRLB = TWI_ACKACT_NACK_gc;
        } else {
            TWI0.MCTRLB = TWI_ACKACT_ACK_gc | TWI_MCMD_RECVTRANS_gc;
        }
    }

    return 0;
}

void i2c_stop(void)
{
    TWI0.MCTRLB |= TWI_MCMD_STOP_gc;
}
