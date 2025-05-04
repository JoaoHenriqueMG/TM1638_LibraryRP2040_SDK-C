#include "pico/stdlib.h"
#include "tm1638.h"
#define buzzer 13
#define rele 14

void init_pins(void);
void set_time(int *tic);
void show_time(int tic);
void format(int value, char *str);
void format(int value, char *str);
void decrement(int *tic);
void buzzer_signal(uint64_t *buzzer_temp);

bool buzzer_on = false;

int main() {
    stdio_init_all();
    tm1638_init(7);
    init_pins();
    int tic = 0;
    int state = 0;
    while (1) {
        set_time(&tic);
        decrement(&tic);
    }
}

void set_time(int *tic) {
    uint key[8];
    tm1638_get_keys(key);
    int tic_ant = *tic;
    show_time(*tic);
    while (!key[0]) {
        tic_ant = *tic;
        tm1638_get_keys(key);
        if (key[3]) {
            while (key[3]) tm1638_get_keys(key);
            *tic = *tic + 300;
        } else if (key[4]) {
            while (key[4]) tm1638_get_keys(key);
            *tic = *tic + 60;
        } else if (key[5]) {
            while (key[5]) tm1638_get_keys(key);
            *tic = *tic + 5;
        } else if (key[6]) {
            while (key[6]) tm1638_get_keys(key);
            *tic = *tic + 1;
        }
        if (*tic < 0) *tic = 0;
        if (key[2]) {
            while (key[2]) tm1638_get_keys(key);;
            *tic = 0;
        }
        if (tic_ant != *tic) {
            show_time(*tic);
            tic_ant = *tic;
        }
        
    }
    while(key[0]) tm1638_get_keys(key);;
}

void show_time(int tic)  {
    int timer[3] = {(tic/3600)%60, (tic/60)%60, tic%60};
    char timer_str[3][3];
    tm1638_show_string("  -  -  ", 0);
    for (int i = 0; i < 3; i++) {
        format(timer[i], timer_str[i]);
        tm1638_show_string(timer_str[i], i*3);
    }
}

void format(int value, char *str){
    if (value > 9) {
        snprintf(str, 3, "%d", value);
    } else {
        snprintf(str, 3, "0%d", value);
    }
}

void decrement(int *tic) {
    gpio_put(rele, 1);
    uint key[8];
    tm1638_get_keys(key);
    int tic_init = *tic;
    bool run = true, press_0 = false, press_1 = false;
    uint64_t last_time = time_us_64(), buzzer_temp = time_us_64();
    show_time(*tic);
    while (!key[7]) {
        tm1638_get_keys(key);
        if (key[0] && !press_0) {
            run = !run;
            gpio_put(rele, run);
            press_0 = true;
        }
        if (!key[0]) press_0 = false;
        if (key[1] && !press_1) {
            *tic = tic_init;
            press_1 = true;
            show_time(*tic);
            last_time = time_us_64();
            gpio_put(rele, 1);
        }
        if (!key[1]) press_1 = false;
        if (*tic != 0) {
            if (run) {
                if (time_us_64() - last_time > 999999) {
                    last_time = time_us_64();
                    *tic -= 1;
                    show_time(*tic);
                }
            } else {
                last_time = time_us_64();
            }
        } else {
            gpio_put(rele, 0);
        }
        if (*tic < 11) {
            gpio_put(buzzer, 0);
        } else if (*tic < 61 && run) {
            buzzer_signal(&buzzer_temp);
        } else if (run) {
            buzzer_on = 0;
            gpio_put(buzzer, 0);
            buzzer_temp = time_us_64();
        } else {
            buzzer_on = 0;
            gpio_put(buzzer, 0);
            buzzer_temp = last_time;
        }
    }
    while (key[7]) tm1638_get_keys(key);
    gpio_put(rele, 0);
    gpio_put(buzzer, 0);
}

void buzzer_signal(uint64_t *buzzer_temp) {
    if (time_us_64() - *buzzer_temp > 499999) {
        gpio_put(buzzer, buzzer_on);
        buzzer_on = !buzzer_on;
        *buzzer_temp = time_us_64();
    }
}

void init_pins(void) {
    gpio_init(rele); gpio_init(buzzer);
    gpio_set_dir(rele, GPIO_OUT); gpio_set_dir(buzzer, GPIO_OUT);
    gpio_put(rele, 0); gpio_put(buzzer, 0);
}
