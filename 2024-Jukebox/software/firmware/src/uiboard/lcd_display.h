#ifndef _LCD_DISPLAY_H
#define _LCD_DISPLAY_H

#define LCD_ROWS 2
#define LCD_COLS 40
#define LCD_B4 13
#define LCD_B5 12
#define LCD_B6 11
#define LCD_B7 10
#define LCD_RS 14

int LCDpins[6] = { LCD_B7, LCD_B6, LCD_B5, LCD_B4, LCD_RS };
uint32_t LCDmask_c = 0; // with clock
uint32_t LCDmask = 0; //without clock

uint32_t _lcd_pin_values_to_mask(uint raw_bits[], int length);
void _lcd_uint_into_8bits(uint raw_bits[], uint one_byte);
void _lcd_send_raw_data_one_cycle(uint8_t en, uint raw_bits[]);
void _lcd_send_full_byte(uint8_t en, uint rs, uint databits[]);
void lcd_clear(uint8_t en);
void lcd_init(uint8_t en);
void lcd_goto_pos(uint8_t en, int pos, int line);
void lcd_print(uint8_t en, const char * str);
void lcd_print_wrapped(uint8_t en, const char * str);

#endif
