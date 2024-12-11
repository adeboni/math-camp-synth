#include "pico/stdlib.h"
#include "lcd_display.h"

const uint8_t _data_pins[4] = { LCD_B4, LCD_B5, LCD_B6, LCD_B7 };
const uint8_t _row_offsets[4] = { 0x00, 0x40, 0x00 + LCD_COLS, 0x40 + LCD_COLS};
uint8_t pins_configured = 0;

void lcd_init(uint8_t enable) {
    if (pins_configured == 0) {
        gpio_init(LCD_RS);
        gpio_set_dir(LCD_RS, GPIO_OUT);
        for (int i = 0; i < 4; i++) {
            gpio_init(_data_pins[i]);
            gpio_set_dir(_data_pins[i], GPIO_OUT);
        }
        pins_configured = 1;
    }
    
    gpio_init(enable);
    gpio_set_dir(enable, GPIO_OUT);
    sleep_us(50000);
    gpio_put(LCD_RS, 0);
    gpio_put(enable, 0);
    sleep_us(50000);
    lcd_write4bits(enable, 0x03);
    sleep_us(4500);
    lcd_write4bits(enable, 0x03);
    sleep_us(4500);
    lcd_write4bits(enable, 0x03);
    sleep_us(150);
    lcd_write4bits(enable, 0x02);

    lcd_send(enable, LCD_FUNCTIONSET | LCD_4BITMODE | LCD_1LINE | LCD_5x8DOTS | LCD_2LINE, 0);  
    lcd_send(enable, LCD_DISPLAYCONTROL | LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKOFF, 0);
    lcd_clear(enable);
    lcd_send(enable, LCD_ENTRYMODESET | LCD_ENTRYLEFT | LCD_ENTRYSHIFTDECREMENT, 0);
}

void lcd_clear(uint8_t enable) {
    lcd_send(enable, LCD_CLEARDISPLAY, 0);
    sleep_us(2000);
}

void lcd_home(uint8_t enable) {
    lcd_send(enable, LCD_RETURNHOME, 0);
    sleep_us(2000);
}

void lcd_print(uint8_t enable, uint8_t chr) {
    lcd_send(enable, chr, 1);
}

void lcd_print_str(uint8_t enable, const char *str) {
    int i = 0;
    while (str[i] != 0) {
        lcd_send(enable, (uint8_t)str[i], 1);
        i++;
    }
}

void lcd_setCursor(uint8_t enable, uint8_t col, uint8_t row) {
    const size_t max_lines = sizeof(_row_offsets) / sizeof(*_row_offsets);
    if (row >= max_lines) row = max_lines - 1;
    if (row >= LCD_ROWS) row = LCD_ROWS - 1;
    lcd_send(enable, LCD_SETDDRAMADDR | (col + _row_offsets[row]), 0);
}

void lcd_createChar(uint8_t enable, uint8_t location, uint8_t charmap[]) {
    location &= 0x7; // we only have 8 locations 0-7
    lcd_send(enable, LCD_SETCGRAMADDR | (location << 3), 0);
    for (int i = 0; i < 8; i++)
        lcd_send(enable, charmap[i], 1);
}

void lcd_send(uint8_t enable, uint8_t value, uint8_t mode) {
    gpio_put(LCD_RS, mode);
    lcd_write4bits(enable, value >> 4);
    lcd_write4bits(enable, value);
}

void lcd_write4bits(uint8_t enable, uint8_t value) {
    for (int i = 0; i < 4; i++)
        gpio_put(_data_pins[i], (value >> i) & 0x01);
    gpio_put(enable, 0);
    sleep_us(1);
    gpio_put(enable, 1);
    sleep_us(1);
    gpio_put(enable, 0);
    sleep_us(100);
}
