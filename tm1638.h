#ifndef TM1628_H
#define TM1628_H
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include <string.h>
#include <stdio.h>
#include "tm1638.c"

void tm1638_init(uint brightness);
void tm1638_clear(void);
void tm1638_set_power(bool on);
void tm1638_show_string(char *string, uint pos);
void tm1638_set_brightness(uint brightnees);
void tm1638_set_led(uint pos, bool on);
void tm1638_set_leds(uint *leds);
void tm1638_get_keys(uint *key);
void tm1638read_keys_mask(void);

#endif
