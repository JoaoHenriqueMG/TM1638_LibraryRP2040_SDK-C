#include "tm1638.h"

#define TM1638_STB 2
#define TM1638_CLK 3
#define TM1638_DIO 4
#define TM1638_CMD1 0x40 // data command
#define TM1638_CMD2 0xC0 // address command
#define TM1638_CMD3 0x80 // display control command
#define TM1638_DPS_ON_CONFIG 0X88 // activates display with configured brightness
#define TM1638_READ_BTN 0x42 // read key

const uint8_t _COD[] = {
    // 0 - 9
    0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07,
    0x7F, 0x6F,
    // A - Z
    0x77, 0x7C, 0x39, 0x5E, 0x79, 0x71,
    0x3D, 0x76, 0x06, 0x1E, 0x76, 0x38, 0x55, 0x54,
    0x3F, 0x73, 0x67, 0x50, 0x6D, 0x78, 0x3E, 0x1C,
    0x2A, 0x76, 0x6E, 0x5B,
    // Espaço, dash and asterisk
    0x00, 0x40, 0x63
};

static uint _stb = TM1638_STB, _clk = TM1638_CLK, _dio = TM1638_DIO;
static uint8_t _brightness, _led_state[8] = {0}, _segments[8] = {0}; 

static void send_byte(uint8_t data) {
    gpio_set_dir(_dio, GPIO_OUT); // configure dio as output
    for (int i = 0; i < 8; ++i) {  // sending 8-bit data
        gpio_put(_clk, 0); 
        gpio_put(_dio, data & 0x01);
        sleep_us(1);
        gpio_put(_clk, 1);
        sleep_us(1);
        data >>= 1; // shift-left on data
    }
}

static void command(uint8_t cmd) {
    gpio_put(_stb, 0);         // activate the chip (STB LOW)
    send_byte(cmd);     // envia comando
    gpio_put(_stb, 1);         // activate the chip (STB HIGH)
}

static void update(void) {
    command(TM1638_CMD1); 
    gpio_put(_stb, 0);  // releases data sending
    send_byte(TM1638_CMD2);
    for (int i = 0; i < 8; i++) {  // updates what should be shown on displays and LEDs
        send_byte(_segments[i]);
        send_byte(_led_state[i]);
    }
    gpio_put(_stb, 1); // end of data sending and start of processing it
}

static uint8_t encode_char(char c) {
    if (c >= '0' && c <= '9') return _COD[c - '0'];
    if (c >= 'A' && c <= 'Z') return _COD[c - 'A'];
    if (c >= 'a' && c <= 'z') return _COD[c - 'a'];
    if (c == ' ') return _COD[36];
    if (c == '-') return _COD[37];
    if (c == '*') return _COD[38];
    return 0X00;
}

static uint8_t read_byte() {
    uint8_t value = 0;
    for (int i = 0; i < 8; i++) { // reading the inputs
        gpio_put(_clk, 0);
        sleep_us(1);
        value |= gpio_get(_dio) << i;
        gpio_put(_clk, 1);
        sleep_us(1);
    }
    return value;
}

static uint8_t read_keys() {
    uint8_t keys = 0;
    gpio_put(_stb, 0);  // realeses data reading
    send_byte(TM1638_READ_BTN);
    gpio_set_dir(_dio, GPIO_IN); // configure dio as input
    for (int i = 0; i < 4; i++) { // organizes the reading of inputs into 8-bit data
        uint8_t byte = read_byte() << i;
        keys |= byte;
    }
    gpio_set_dir(_dio, GPIO_OUT);
    gpio_put(_stb, 1);  // end of data reading
    return keys;
}

void tm1638_init(uint brightness) {
    _brightness = brightness > 7 ? 7 : brightness;
    gpio_init(_stb); gpio_set_dir(_stb, GPIO_OUT);
    gpio_init(_clk); gpio_set_dir(_clk, GPIO_OUT);
    gpio_init(_dio); gpio_set_dir(_dio, GPIO_OUT);
    gpio_put(_stb, 1);
    gpio_put(_clk, 1);
    gpio_put(_dio, 1);
    
    command(TM1638_CMD1);

    // clear all 16 RAM locations with zeros
    gpio_put(_stb, 0);
    send_byte(TM1638_CMD2 | 0x00); // init address 0x00
    for (int i = 0; i < 16; ++i) {
        send_byte(0x00);
    }
    gpio_put(_stb, 1);

    command(TM1638_DPS_ON_CONFIG | _brightness); // turns on the LED with a the desired brightness
}

void tm1638_clear(void) {  // clears the memory by adding spaces on all locations
    for (int i = 0; i < 8; i++){
        _segments[i] = 0x00;
        _led_state[i] = 0x00;
    }
    update();
}

void tm1638_show_string(char *string, uint pos)  {
    if (pos < 0 || pos > 7) return; 
    for (int i = pos; i < 8 && string[i - pos]; i++){  // converts characters to their respective hexadecimal representations
        if (string[i - pos]) _segments[i] = encode_char(string[i - pos]); // add on vector 
    }
    update();
}

void tm1638_set_power(bool on){
    if (on)
        command(TM1638_DPS_ON_CONFIG | _brightness);
    else
        command(TM1638_CMD3); // display off 
}

void tm1638_set_brightness(uint brightnees){  // set display with a desired brightness
    _brightness = brightnees > 7 ? 7 : brightnees;
    command(TM1638_DPS_ON_CONFIG | _brightness);
}

void tm1638_set_led(uint pos, bool on) {  // turns a led on or off
    if (pos > 7) return;
    _led_state[pos] = on;
    update();
}

void tm1638_set_leds(uint *leds) {  //  set the state of all LEDs, from a vector
    for (int i = 0; i < 8; i++){
        _led_state[i] = leds[i];
    }
    update();
}

void tm1638_set_leds_mask(uint8_t mask){ //  set the state of all LEDs, from a mask
    for (int i = 0; i < 8; i++){
        _led_state[i] = (mask >> (7 - i)) & 0x01;
    }
    update();
}

uint8_t tm1638_read_keys_mask(void) {  // read the keys and returns a mask
    uint8_t mask = read_keys();
    uint8_t mask_t;
    for (int i = 0; i < 8; i++){
        mask_t |= ((mask & (1 << i)) >> i) << (7 - i);  // reverses the order of the bits
    }
    return mask_t;
}

void tm1638_get_keys(uint *key) {  // read the keys and returns a vector
    uint8_t keys = read_keys();
    for (int i = 7; i > -1; i--) {
        key[i] = (keys >> i) & 1;  // adds bits on vector (LSB in first position and MSB in last position)
    }
}
