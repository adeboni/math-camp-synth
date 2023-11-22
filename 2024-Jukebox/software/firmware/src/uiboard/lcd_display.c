#include "pico/stdlib.h"
#include "lcd_display.h"

uint32_t _lcd_pin_values_to_mask(uint raw_bits[], int length) {   // Array of Bit 7, Bit 6, Bit 5, Bit 4, RS(, clock)
    uint32_t result = 0;
    uint pinArray[32];
    for (int i = 0; i < 32; i++) 
        pinArray[i] = 0;
    for (int i = 0; i < length; i++) 
        pinArray[LCDpins[i]] = raw_bits[i];
    for (int i = 0; i < 32; i++) {
        result = result << 1;
        result += pinArray[31-i];
    }
    return result;
}

void _lcd_uint_into_8bits(uint raw_bits[], uint one_byte) {  	
    for (int i = 0; i < 8; i++) {
        raw_bits[7-i] = one_byte % 2;
        one_byte = one_byte >> 1;
    }
}

void _lcd_send_raw_data_one_cycle(uint8_t en, uint raw_bits[]) { // Array of Bit 7, Bit 6, Bit 5, Bit 4, RS
    uint32_t bit_value = pin_values_to_mask(raw_bits, 5);
    gpio_put_masked(LCDmask, bit_value);
    gpio_put(en, 1);
    sleep_ms(5);
    gpio_put(en, 0); // gpio values on other pins are pushed at the HIGH->LOW change of the clock. 
    sleep_ms(5);
}
    
void _lcd_send_full_byte(uint8_t en, uint rs, uint databits[]) { // RS + array of Bit7, ... , Bit0
    // send upper nibble (MSN)
    uint rawbits[5];
    rawbits[4] = rs;
    for (int i = 0; i < 4; i++) 
        rawbits[i] = databits[i];
    _lcd_send_raw_data_one_cycle(en, rawbits);
    // send lower nibble (LSN)
    for (int i = 0; i < 4; i++) 
        rawbits[i] = databits[i + 4];
    _lcd_send_raw_data_one_cycle(en, rawbits);
}

void lcd_clear(uint8_t en) {
    uint clear_display[8] = { 0, 0, 0, 0, 0, 0, 0, 1 };
    _lcd_send_full_byte(en, 0, clear_display);
    sleep_ms(10); // extra sleep due to equipment time needed to clear the display
}

void lcd_init(uint8_t en) { // initialize the LCD
    uint all_ones[6] = { 1, 1, 1, 1, 1, 1 };
    uint set_function_8[5] = { 0, 0, 1, 1, 0 };
    uint set_function_4a[5] = { 0, 0, 1, 0, 0 };
    
    uint set_function_4[8] = { 0, 0, 1, 0, 0, 0, 0, 0 };
    uint cursor_set[8] = { 0, 0, 0, 0, 0, 1, 1, 0 };
    uint display_prop_set[8] = { 0, 0, 0, 0, 1, 1, 0, 0 };
    
    //set mask, initialize masked pins and set to LOW 
    LCDpins[5] = en;
    LCDmask_c = _lcd_pin_values_to_mask(all_ones, 6);
    LCDmask = _lcd_pin_values_to_mask(all_ones, 5);
    gpio_init_mask(LCDmask_c);   	    // init all LCDpins
    gpio_set_dir_out_masked(LCDmask_c);	// Set as output all LCDpins
    gpio_clr_mask(LCDmask_c);			// LOW on all LCD pins 
    
    //set LCD to 4-bit mode and 1 or 2 lines
    //by sending a series of Set Function 0s to secure the state and set to 4 bits
    if (LCD_ROWS == 2) 
        set_function_4[4] = 1;
    _lcd_send_raw_data_one_cycle(en, set_function_8);
    _lcd_send_raw_data_one_cycle(en, set_function_8);
    _lcd_send_raw_data_one_cycle(en, set_function_8);
    _lcd_send_raw_data_one_cycle(en, set_function_4a);
    
    //getting ready
    _lcd_send_full_byte(en, 0, set_function_4);
    _lcd_send_full_byte(en, 0, cursor_set);
    _lcd_send_full_byte(en, 0, display_prop_set);
    lcd_clear(en);
}

void lcd_goto_pos(uint8_t en, int pos_i, int line) {
    uint eight_bits[8];
    uint pos = (uint)pos_i;
    switch (LCD_ROWS) {
        case 2: 
            pos = 64 * line + pos + 0b10000000; 
            break;
        case 4: 	
            if (line == 0 || line == 2)
                pos = 64 * (line/2) + pos + 0b10000000;
            else 
                pos = 64 * ((line-1)/2) + 20 + pos + 0b10000000;
            break;
        default:
            break;
    }
    _lcd_uint_into_8bits(eight_bits, pos);
    _lcd_send_full_byte(en, 0, eight_bits);
}

void lcd_print(uint8_t en, const char * str) {
    uint eight_bits[8];
    int i = 0;
    while (str[i] != 0) {
        _lcd_uint_into_8bits(eight_bits, (uint)(str[i]));
        _lcd_send_full_byte(en, 1, eight_bits);
        i++;
    }
}
    
void lcd_print_wrapped(uint8_t en, const char * str) {
    uint eight_bits[8];
    int i = 0;
    
    _lcd_goto_pos(en, 0, 0);

    while (str[i] != 0) {
        _lcd_uint_into_8bits(eight_bits, (uint)(str[i]));
        _lcd_send_full_byte(en, 1, eight_bits);
        i++;
        if (i % no_chars == 0)
            lcd_goto_pos(en, 0, i / no_chars);
    }
}
