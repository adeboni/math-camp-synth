#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/i2c.h"
#include "hardware/clocks.h"
#include "hardware/adc.h"
#include "wizchip_conf.h"
#include "w5x00_spi.h"
#include "e131.h"
#include "MAX7313.h"
#include "bsp/board.h"
#include "tusb.h"
#include "usb_descriptors.h"
#include "lcd_display.h"

#define HID_UPDATE_MS 10
#define MODE_UPDATE_MS 2000
#define DISPLAY_UPDATE_MS 100
#define LCD_EN0 9
#define LCD_EN1 15
#define MAX7313_ADDR0 0x20
#define MAX7313_ADDR1 0x21
#define MAX7313_ADDR2 0x23
#define VOLUME_PIN 26
#define PLL_SYS_KHZ (133 * 1000)

// MAX7313 pin numbers
uint8_t DOTS_OUT[6]    = {7, 6, 4, 2, 0, 15};        //ADDR1
uint8_t BUTTONS_OUT[7] = {14, 13, 12, 11, 10, 9, 8}; //ADDR1
uint8_t MOTORS_OUT[3]  = {5, 3, 1};                  //ADDR1
uint8_t LAMP_OUT[1]    = {8};                        //ADDR0
uint8_t MOUTH_OUT[15]  = {13, 12, 11, 10, 9, //RED     ADDR0
                          7, 5, 3, 1, 15,    //WHITE
                          6, 4, 2, 0, 14};   //BLUE
uint8_t MODE_IN[8]     = {7, 5, 3, 1, 0, 6, 4, 2};   //ADDR2
uint8_t BUTTONS_IN[7]  = {14, 13, 12, 11, 10, 9, 8}; //ADDR2
uint8_t MODE_KEYS[8] = {HID_KEY_8, HID_KEY_7, HID_KEY_6, HID_KEY_5, 
                        HID_KEY_4, HID_KEY_3, HID_KEY_2, HID_KEY_1};
uint8_t BUTTON_KEYS[7] = {HID_KEY_SPACE, HID_KEY_ARROW_LEFT, 
                          HID_KEY_ARROW_DOWN, HID_KEY_ARROW_RIGHT,
                          HID_KEY_ENTER, HID_KEY_ARROW_UP, HID_KEY_POWER};

uint8_t target_volume = 0;
uint8_t current_volume = 0;

static void set_clock_khz(void) {
    set_sys_clock_khz(PLL_SYS_KHZ, true);
    clock_configure(
        clk_peri,
        0,                                                // No glitchless mux
        CLOCKS_CLK_PERI_CTRL_AUXSRC_VALUE_CLKSRC_PLL_SYS, // System PLL on AUX mux
        PLL_SYS_KHZ * 1000,                               // Input frequency
        PLL_SYS_KHZ * 1000                                // Output (must be same as no divider)
    );
}

uint32_t millis() {
    return to_ms_since_boot(get_absolute_time());
}

uint8_t get_board_id() {
    gpio_init(6);
    gpio_set_dir(6, GPIO_IN);
    gpio_pull_up(6);
    gpio_init(7);
    gpio_set_dir(7, GPIO_IN);
    gpio_pull_up(7);
    gpio_init(8);
    gpio_set_dir(8, GPIO_IN);
    gpio_pull_up(8);
    uint8_t b0 = 1 - gpio_get(6);
    uint8_t b1 = 1 - gpio_get(7);
    uint8_t b2 = 1 - gpio_get(8);
    return b0 | (b1 << 1) | (b2 << 2);
}

void w5500_init() {
    wizchip_spi_initialize();
    wizchip_cris_initialize();
    wizchip_reset();
    wizchip_initialize();
    wizchip_check();

    //uint8_t board_id = get_board_id();
    uint8_t board_id = 0;
    wiz_NetInfo g_net_info = {
        .mac = {0x00, 0x08, 0xDC, 0x12, 0x34, 0x20 + board_id}, // MAC address
        .ip = {10, 0, 0, 20 + board_id},                        // IP address
        .sn = {255, 255, 255, 0},                               // Subnet Mask
        .gw = {10, 0, 0, 1},                                    // Gateway
        .dns = {8, 8, 8, 8},                                    // DNS server
        .dhcp = NETINFO_STATIC                                  // DHCP enable/disable
    };

    network_initialize(g_net_info);
}

void _tud_task() {
    do {
        tud_task();
        if (tud_suspended()) 
            tud_remote_wakeup();
    } while (!tud_hid_ready());
}

void key_task(uint8_t scancode) {
    uint8_t keys[6] = { scancode };
    _tud_task();
    tud_hid_keyboard_report(REPORT_ID_KEYBOARD, 0, keys);
    _tud_task();
    tud_hid_keyboard_report(REPORT_ID_KEYBOARD, 0, NULL);
}

void volume_task() {
    uint8_t vol_inc = 2;
    uint8_t vol_dir = current_volume < target_volume ? HID_USAGE_CONSUMER_VOLUME_INCREMENT : HID_USAGE_CONSUMER_VOLUME_DECREMENT;
    uint16_t empty_key = 0;

    while (abs(current_volume - target_volume) >= vol_inc) {
        _tud_task();
        tud_hid_report(REPORT_ID_CONSUMER_CONTROL, &vol_dir, 2);
        _tud_task();
        tud_hid_report(REPORT_ID_CONSUMER_CONTROL, &empty_key, 2);
        current_volume += vol_dir == HID_USAGE_CONSUMER_VOLUME_INCREMENT ? vol_inc : -vol_inc;
    }
}

void hid_task() {
    static uint32_t start_ms = 0;
    static uint8_t mode_states[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
    static uint8_t button_states[7] = { 0, 0, 0, 0, 0, 0, 0 };

    _tud_task();
    if (millis() - start_ms > HID_UPDATE_MS) { 
        start_ms = millis();

        for (int i = 0; i < 8; i++) {
            uint8_t state = 1 - max7313_digitalRead(MAX7313_ADDR2, MODE_IN[i]);
            if (mode_states[i] == 0 && state == 1) 
                key_task(MODE_KEYS[i]);
            mode_states[i] = state;
        }

        for (int i = 0; i < 7; i++) {
            uint8_t state = 1 - max7313_digitalRead(MAX7313_ADDR2, BUTTONS_IN[i]);
            if (button_states[i] == 0 && state == 1) 
                key_task(BUTTON_KEYS[i]);
            button_states[i] = state;
        }

        uint16_t sum = 0;
        for (int i = 0; i < 5; i++)
            sum += adc_read();
        sum /= 5;
        target_volume = 100 - sum / 41;
        volume_task();
    }
}

void update_display(const e131_packet_t *packet, uint8_t start, uint8_t end, uint8_t en) {
    for (int i = start; i < end; i++)
        lcd_print(en, packet->dmp.prop_val[i]);
}

void update_leds(const e131_packet_t *packet, uint8_t start, uint8_t end, uint8_t i2c_addr, uint8_t *pins) {
    for (int i = start; i < end; i++)
        max7313_analogWrite(i2c_addr, pins[i - start], packet->dmp.prop_val[i] >> 4);
}

void show_message(const char *str) {
    lcd_clear(LCD_EN0);
    lcd_home(LCD_EN0);
    lcd_print_str(LCD_EN0, str);
}

int main() {
    set_clock_khz();
    stdio_init_all();
    adc_init();
    
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    gpio_put(PICO_DEFAULT_LED_PIN, 0);

    lcd_init(LCD_EN0);
    lcd_init(LCD_EN1);

    adc_gpio_init(VOLUME_PIN);
    adc_select_input(0);

    i2c_init(I2C_INST, 400000);
    gpio_set_function(SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(SDA_PIN);
    gpio_pull_up(SCL_PIN);

    max7313_init(MAX7313_ADDR0, 0, 0);
    max7313_init(MAX7313_ADDR1, 0, 0);
    max7313_init(MAX7313_ADDR2, 255, 255);

    for (int i = 0; i < 16; i++) {
        max7313_analogWrite(MAX7313_ADDR0, i, 0);
        max7313_analogWrite(MAX7313_ADDR1, i, 0);
    }
    
    show_message("Initializing hardware");
    
    w5500_init();
    board_init();
    tusb_init();

    show_message("Waiting for boot to finish");

    e131_packet_t packet;
    uint32_t last_display_update = 0;
    uint32_t last_mode_update = 0;
    while (1) {
        hid_task();
        
        if (e131_socket())
            continue;

        if (e131_recv(&packet) <= 0)
            continue;

        if (e131_pkt_validate(&packet) != E131_ERR_NONE)
            continue;

        if (packet.dmp.prop_val_cnt < 193)
            continue;

        if (millis() - last_display_update > DISPLAY_UPDATE_MS) {
            lcd_setCursor(LCD_EN0, 0, 0);
            update_display(&packet, 1, 41, LCD_EN0);
            lcd_setCursor(LCD_EN0, 0, 1);
            update_display(&packet, 41, 81, LCD_EN0);
            lcd_setCursor(LCD_EN1, 0, 0);
            update_display(&packet, 81, 121, LCD_EN1);
            lcd_setCursor(LCD_EN1, 0, 1);
            update_display(&packet, 121, 161, LCD_EN1);
            last_display_update = millis();
        }

        update_leds(&packet, 161, 176, MAX7313_ADDR0, MOUTH_OUT);
        update_leds(&packet, 176, 182, MAX7313_ADDR1, DOTS_OUT);
        update_leds(&packet, 182, 185, MAX7313_ADDR1, MOTORS_OUT);
        update_leds(&packet, 185, 186, MAX7313_ADDR0, LAMP_OUT);
        update_leds(&packet, 186, 193, MAX7313_ADDR1, BUTTONS_OUT);
    }
}

void tud_hid_report_complete_cb(uint8_t instance, uint8_t const* report, uint16_t len) {
    (void) instance;
    (void) report;
    (void) len;
}

void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize) {
    (void) instance;
    (void) report_id;
    (void) report_type;
    (void) buffer;
    (void) bufsize;
}

uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen) {
    (void) instance;
    (void) report_id;
    (void) report_type;
    (void) buffer;
    (void) reqlen;
    return 0;
}