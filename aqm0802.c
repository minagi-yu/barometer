#include "aqm0802.h"
#include "i2c.h"

#define AQM0802_RS_CMD 0x00
#define AQM0802_RS_DAT 0x40

#define ST7032_CLR_DISP 0x01
#define ST7032_RET_HOME 0x02
#define ST7032_MODE_SET 0x04
#define ST7032_MODE_SET_ID 0x20
#define ST7032_MODE_SET_S 0x01
#define ST7032_DISP_ON 0x08
#define ST7032_DISP_ON_D 0x04
#define ST7032_DISP_ON_C 0x02
#define ST7032_DISP_ON_P 0x01
#define ST7032_FUNC_SET 0x20
#define ST7032_FUNC_SET_DL 0x10
#define ST7032_FUNC_SET_N 0x08
#define ST7032_FUNC_SET_DH 0x04
#define ST7032_FUNC_SET_IS 0x01

static void aqm0802_delay_us(uint16_t us)
{
    do {
        asm("nop");
        asm("nop");
        asm("nop");
    } while (--us);
}

static void aqm0802_write(uint8_t rs, uint8_t data)
{
    uint8_t d[2];
    d[0] = rs;
    d[1] = data;
    i2c_write(0x3e, &d, 2);
    i2c_stop();
}

void aqm0802_clear(void)
{
    aqm0802_write(AQM0802_RS_CMD, 0x01);
    aqm0802_delay_us(1100);
}

void aqm0802_init(void)
{
    aqm0802_write(AQM0802_RS_CMD, 0x38);
    aqm0802_delay_us(30);
    aqm0802_write(AQM0802_RS_CMD, 0x39);
    aqm0802_delay_us(30);
    aqm0802_write(AQM0802_RS_CMD, 0x14);
    aqm0802_delay_us(30);
    aqm0802_write(AQM0802_RS_CMD, 0x70);
    aqm0802_delay_us(30);
    aqm0802_write(AQM0802_RS_CMD, 0x56);
    aqm0802_delay_us(30);
    aqm0802_write(AQM0802_RS_CMD, 0x6c);
    aqm0802_delay_us(30);
    aqm0802_write(AQM0802_RS_CMD, 0x38);
    aqm0802_delay_us(30);
    aqm0802_write(AQM0802_RS_CMD, 0x0c);
    aqm0802_delay_us(30);
    aqm0802_clear();
}

// void aqm0802_power_off(void)
// {
//     aqm0802_write(AQM0802_RS_CMD, 0x08); // Display OFF
//     aqm0802_delay_us(30);
//     aqm0802_write(AQM0802_RS_CMD, 0x39); // Function 1
//     aqm0802_delay_us(30);
//     aqm0802_write(AQM0802_RS_CMD, 0x60); // Follower circuit OFF
//     aqm0802_delay_us(30);
//     aqm0802_write(AQM0802_RS_CMD, 0x50); // Booster circuit OFF
//     aqm0802_delay_us(30);
//     aqm0802_write(AQM0802_RS_CMD, 0x10); // Booster circuit OFF
//     aqm0802_delay_us(30);
// }

void aqm0802_locate(uint8_t row, uint8_t col)
{
    aqm0802_write(AQM0802_RS_CMD, 0x80 | ((0x40 * row) + col));
    aqm0802_delay_us(30);
}

void aqm0802_putc(char c)
{
    aqm0802_write(AQM0802_RS_DAT, c);
}
