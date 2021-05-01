#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/sleep.h>
#include <util/delay.h>
#include "aqm0802.h"
#include "dps368.h"
#include "i2c.h"

/* ATtiny402 Pin Connection
 * 1: VDD   P   VDD
 * 2: PA6   I   DPS368_SDO
 * 3: PA7   I   Switch
 * 4: PA1   -   I2C_SDA
 * 5: PA2   -   I2C_SCL
 * 6: PA0   I   UPDI
 * 7: PA3   O   VDD_OUT
 * 8: GND   P   GND
 *
 * DPS368 Pin Connection
 * 1: GND   GND
 * 2: CSB   NC
 * 3: SDI   I2C_SDA
 * 4: SCK   I2C_SCK
 * 5: SDO   Interrupt
 * 6: VDDIO VDD_OUT
 * 7: GND   GND
 * 8: VDD   VDD_OUT
 *
 * AQM0802A Pin Connection
 * 1: VO    NC
 * 2: VOUT  C1
 * 3: CAPIN C2
 * 4: CAPIP C2
 * 5: VDD   VDD_OUT
 * 6: VSS   GND
 * 7: SDA   I2C_SDA
 * 8: SCL   I2C_SCL
 * 9: XRSETB    VDD_OUT
 */

// offtime: 1875 * 16ms = 30s
#define AUTO_OFF_TIME 1875

#define DEVICE_POWER_ON() PORTA.OUTSET = PIN3_bm;
#define DEVICE_POWER_OFF() PORTA.OUTCLR = PIN3_bm;

#define CMD_ENTER 0x01  // Long push
#define CMD_SELECT 0x02 // Single push
#define CMD_READ 0x04   // Sensor ready

#define DISP_DEFAULT 0
#define DISP_SUB 1

#define UNIT_DC 0
#define UNIT_CM 1
#define UNIT_PA 2

volatile uint16_t uptime;
volatile uint8_t cmd;

void delay_16ms(uint16_t t)
{
    t += uptime;
    do {
        sleep_mode();
    } while (uptime < t);
}

void puts8(char *s, uint8_t i)
{
    for (uint8_t j = 0; j < (8 - i); j++) {
        aqm0802_putc(' ');
    }
    do {
        aqm0802_putc(s[--i]);
    } while (i);
}

void print(int32_t prs, uint8_t unit)
{
    char s[8], sign = 0;
    uint8_t i = 0;

    if (prs < 0) {
        sign = '-';
        prs = -prs;
    }

    if (unit == UNIT_PA) {
        s[i++] = 'a';
        s[i++] = 'P';
        aqm0802_locate(0, 0);
    } else {
        if (unit) { // UNIT_CM
            s[i++] = 'm';
            s[i++] = 'c';
        } else {
            s[i++] = 'C';
            s[i++] = 0xf2;
            s[3] = '0';
            s[4] = '.';
            s[5] = '0';
        }
        aqm0802_locate(1, 0);
    }

    do {
        if ((unit == UNIT_DC) && (i == 4)) {
            i++;
            continue;
        }
        s[i++] = (prs % 10) + '0';
        prs /= 10;
    } while ((prs != 0) && (i < sizeof(s)));

    if ((unit == UNIT_DC) && (i < 7))
        i = 6;

    if (sign)
        s[i++] = sign;

    puts8(s, i);
}

int main(void)
{
    uint8_t disp_state = 0;
    struct value {
        int32_t temp;
        int32_t pres;
    } current, saved;

    saved.pres = 0;

    PORTA.DIRSET = PIN3_bm;
    PORTA.PIN7CTRL |= PORT_PULLUPEN_bm;

    i2c_init();

    PORTA.PIN7CTRL |= PORT_ISC_LEVEL_gc;
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    sei();
    sleep_mode();

    for (;;) {
        RTC.CLKSEL = RTC_CLKSEL_INT32K_gc;
        RTC.PITCTRLA = RTC_PERIOD_CYC512_gc | RTC_PITEN_bm;
        RTC.PITINTCTRL = RTC_PI_bm;

        cli();
        uptime = 0;
        cmd = 0;
        sei();
        disp_state = DISP_SUB;

        DEVICE_POWER_ON();
        delay_16ms(5); // Wait device initialization
        aqm0802_init();
        dps368_init();
        dps368_config_prs(DPS368_MEAS_RATE_1, DPS368_SAMP_RATE_16);
        dps368_config_tmp(DPS368_MEAS_RATE_1, DPS368_SAMP_RATE_1);
        dps368_config_int(DPS368_INT_TMP);
        dps368_set_opmode(DPS368_CONT_PRS_TMP_MEAS);
        PORTA.PIN6CTRL |= PORT_ISC_FALLING_gc;

        while (uptime < AUTO_OFF_TIME) {
            uint8_t c;
            cli();
            c = cmd;
            cmd = 0;
            sei();
            if (c) {
                if (c & CMD_READ) {
                    dps368_clear_intflgs();
                    dps368_get_result(&current.temp, &current.pres);
                    if (saved.pres == 0)
                        saved.pres = current.pres;
                }
                if (c & CMD_SELECT) {
                    disp_state ^= 1; // Toggle disp_state
                }
                switch (disp_state) {
                    case DISP_DEFAULT:
                        print(current.pres, UNIT_PA);
                        print(current.temp, UNIT_DC);
                        break;
                    case DISP_SUB:
                        if (c & CMD_ENTER) {
                            saved = current;
                        }
                        int32_t sub = saved.pres - current.pres;
                        print(-sub, UNIT_PA);
                        print(sub * 8.7f, UNIT_CM);
                        break;
                    default:
                        break;
                }
            }
            sleep_mode();
        }

        RTC.CLKSEL = 0;
        RTC.PITCTRLA = 0;
        RTC.PITINTCTRL = 0;
        // pin change interrupt (dps368) disable
        PORTA.PIN6CTRL &= ~PORT_ISC_gm;
        dps368_set_opmode(DPS368_OPMODE_IDOL);
        DEVICE_POWER_OFF();
        PORTA.PIN7CTRL |= PORT_ISC_LEVEL_gc;
        sleep_mode();
    }
}

ISR(PORTA_PORT_vect)
{
    if (PORTA.INTFLAGS & PIN6_bm) {
        // Sensor Ready
        cmd |= CMD_READ;
    }

    // Disable port interrupt
    PORTA.PIN7CTRL &= ~PORT_ISC_gm;
    // Clear interrupt flags
    PORTA.INTFLAGS = PIN6_bm | PIN7_bm;
}

ISR(RTC_PIT_vect)
{
    static uint8_t pin, pin_old;
    static uint16_t push_time;

    pin = PORTA.IN & PIN7_bm;
    if (!pin) { // Switch is pushed
        push_time++;
        uptime = 0;
    }
    if (pin != pin_old) {   // Switch is released
        if (push_time > 50) // Long push
            cmd |= CMD_ENTER;
        else if (push_time > 1) // Short push
            cmd |= CMD_SELECT;
        push_time = 0;
    }

    uptime++;

    // Clear interrupt flags
    RTC.PITINTFLAGS = RTC_PI_bm;
}
