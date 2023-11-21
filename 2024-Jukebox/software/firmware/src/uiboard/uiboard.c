#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/i2c.h"
#include "hardware/clocks.h"
#include "wizchip_conf.h"
#include "w5x00_spi.h"
#include "e131.h"
#include "MAX7313.h"

#define MAX7313_ADDR0 0x20
#define MAX7313_ADDR1 0x21
#define MAX7313_ADDR2 0x23

#define BOARD_ID 0 // change this to read from the dip switches

#define PLL_SYS_KHZ (133 * 1000)

static wiz_NetInfo g_net_info = {
    .mac = {0x00, 0x08, 0xDC, 0x12, 0x34, 0x20 + BOARD_ID}, // MAC address
    .ip = {10, 0, 0, 20 + BOARD_ID},                        // IP address
    .sn = {255, 255, 255, 0},                               // Subnet Mask
    .gw = {10, 0, 0, 1},                                    // Gateway
    .dns = {8, 8, 8, 8},                                    // DNS server
    .dhcp = NETINFO_STATIC                                  // DHCP enable/disable
};

// MAX7313 pin numbers
uint8_t DOTS_OUT[6]    = {7, 6, 4, 2, 0, 15};        //ADDR1
uint8_t BUTTONS_OUT[7] = {14, 13, 12, 11, 10, 9, 8}; //ADDR1
uint8_t MOTORS_OUT[3]  = {5, 3, 1}                   //ADDR1
uint8_t LAMP_OUT[1]    = {8}                         //ADDR0
uint8_t MOUTH_OUT[15]  = {13, 12, 11, 10, 9, //RED     ADDR0
                          7, 5, 3, 1, 15,    //WHITE
                          6, 4, 2, 0, 14}    //BLUE
uint8_t MODE_IN[8]     = {7, 5, 3, 1, 0, 6, 4, 2}    //ADDR2
uint8_t BUTTONS_IN[7]  = {14, 13, 12, 11, 10, 9, 8}  //ADDR2

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

void init_w5500() {
    wizchip_spi_initialize();
	wizchip_cris_initialize();
	wizchip_reset();
	wizchip_initialize();
	wizchip_check();
	network_initialize(g_net_info);
	//print_network_information(g_net_info);
}

int main() {
    set_clock_khz();
    stdio_init_all();

    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    gpio_put(PICO_DEFAULT_LED_PIN, 0);

    max7313_init(MAX7313_ADDR0);
    max7313_init(MAX7313_ADDR1);
    max7313_init(MAX7313_ADDR2);

    for (int i = 0; i < 16; i++) {
        max7313_pinMode(MAX7313_ADDR0, i, MAX7313_PINMODE_OUTPUT);
        max7313_pinMode(MAX7313_ADDR1, i, MAX7313_PINMODE_OUTPUT);
        max7313_pinMode(MAX7313_ADDR2, i, MAX7313_PINMODE_INPUT);

        max7313_analogWrite(MAX7313_ADDR0, i, 0);
        max7313_analogWrite(MAX7313_ADDR1, i, 0);
    }

    // init_w5500();

    while (1) {
        for (int j = 0; j < 6; j++) {
            for (int i = 0; i < 16; i++) {
                max7313_analogWrite(MAX7313_ADDR1, DOTS_OUT[j], i);
                sleep_ms(50);
            }
            for (int i = 0; i < 16; i++) {
                max7313_analogWrite(MAX7313_ADDR1, DOTS_OUT[j], 15 - i);
                sleep_ms(50);
            }
        }

        for (int j = 0; j < 15; j++) {
            for (int i = 0; i < 16; i++) {
                max7313_analogWrite(MAX7313_ADDR0, MOUTH_OUT[j], i);
                sleep_ms(50);
            }
            for (int i = 0; i < 16; i++) {
                max7313_analogWrite(MAX7313_ADDR0, MOUTH_OUT[j], 15 - i);
                sleep_ms(50);
            }
        }
    }

    
    // int sockfd;
    // e131_packet_t packet;
    // e131_error_t error;
    // uint8_t last_seq = 0x00;

    // if ((sockfd = e131_socket()) < 0) {
    // 	printf("e131_socket error\n");
    // 	while (1);
    // }

    // while (1)
    // {
    // 	if (e131_recv(sockfd, &packet) <= 0)
    // 		continue;

    // 	if ((error = e131_pkt_validate(&packet)) != E131_ERR_NONE) {
    // 		//printf("e131_pkt_validate: %s\n", e131_strerror(error));
    // 		continue;
    // 	}

    // 	if (e131_pkt_discard(&packet, last_seq)) {
    // 		//printf("warning: packet out of order received\n");
    // 		last_seq = packet.frame.seq_number;
    // 		continue;
    // 	}

    // 	//e131_pkt_dump(&packet);
    // 	last_seq = packet.frame.seq_number;
    // }

}