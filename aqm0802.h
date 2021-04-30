#ifndef AQM0802_H
#define AQM0802_H

#include <stdint.h>

void aqm0802_clear(void);
void aqm0802_init(void);
// void aqm0802_power_off(void);
void aqm0802_locate(uint8_t row, uint8_t col);
void aqm0802_putc(char c);

#endif