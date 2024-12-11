#ifndef _LCD_DISPLAY_H
#define _LCD_DISPLAY_H

#define LCD_ROWS 2
#define LCD_COLS 40
#define LCD_B4 13
#define LCD_B5 12
#define LCD_B6 11
#define LCD_B7 10
#define LCD_RS 14

// commands
#define LCD_CLEARDISPLAY 0x01
#define LCD_RETURNHOME 0x02
#define LCD_ENTRYMODESET 0x04
#define LCD_DISPLAYCONTROL 0x08
#define LCD_CURSORSHIFT 0x10
#define LCD_FUNCTIONSET 0x20
#define LCD_SETCGRAMADDR 0x40
#define LCD_SETDDRAMADDR 0x80

// flags for display entry mode
#define LCD_ENTRYRIGHT 0x00
#define LCD_ENTRYLEFT 0x02
#define LCD_ENTRYSHIFTINCREMENT 0x01
#define LCD_ENTRYSHIFTDECREMENT 0x00

// flags for display on/off control
#define LCD_DISPLAYON 0x04
#define LCD_DISPLAYOFF 0x00
#define LCD_CURSORON 0x02
#define LCD_CURSOROFF 0x00
#define LCD_BLINKON 0x01
#define LCD_BLINKOFF 0x00

// flags for display/cursor shift
#define LCD_DISPLAYMOVE 0x08
#define LCD_CURSORMOVE 0x00
#define LCD_MOVERIGHT 0x04
#define LCD_MOVELEFT 0x00

// flags for function set
#define LCD_8BITMODE 0x10
#define LCD_4BITMODE 0x00
#define LCD_2LINE 0x08
#define LCD_1LINE 0x00
#define LCD_5x10DOTS 0x04
#define LCD_5x8DOTS 0x00

void lcd_init(uint8_t enable);
void lcd_clear(uint8_t enable);
void lcd_home(uint8_t enable);
void lcd_print(uint8_t enable, uint8_t chr);
void lcd_print_str(uint8_t enable, const char *str);
void lcd_setCursor(uint8_t enable, uint8_t col, uint8_t row);
void lcd_createChar(uint8_t enable, uint8_t location, uint8_t charmap[]);
void lcd_send(uint8_t enable, uint8_t value, uint8_t mode);
void lcd_write4bits(uint8_t enable, uint8_t value);

#endif