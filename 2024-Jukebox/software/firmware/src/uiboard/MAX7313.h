#ifndef _MAX_7313_H
#define _MAX_7313_H

#include "hardware/i2c.h"

/**
  * @note 		datasheet p.13 table 2. Register Address Map
  * @see 		https://datasheets.maximintegrated.com/en/ds/MAX7313.pdf
  */
#define MAX7313_READ_IN_00_07       	0x00
#define MAX7313_READ_IN_08_15       	0x01
#define MAX7313_BLINK_PHASE_0_00_07 	0x02
#define MAX7313_BLINK_PHASE_0_08_15 	0x03
#define MAX7313_PORTS_CONF_00_07    	0x06
#define MAX7313_PORTS_CONF_08_15    	0x07
#define MAX7313_BLINK_PHASE_1_00_07 	0x0A
#define MAX7313_BLINK_PHASE_1_08_15 	0x0B
#define MAX7313_OUT_INT_MA_16       	0x0E
#define MAX7313_CONFIGURATION       	0x0F
#define MAX7313_OUT_INT_01_00       	0x10
#define MAX7313_OUT_INT_03_02       	0x11
#define MAX7313_OUT_INT_05_04       	0x12
#define MAX7313_OUT_INT_07_06       	0x13
#define MAX7313_OUT_INT_09_08       	0x14
#define MAX7313_OUT_INT_11_10       	0x15
#define MAX7313_OUT_INT_13_12       	0x16
#define MAX7313_OUT_INT_15_14       	0x17
#define MAX7313_NO_PORT    	        	0x88

#define MAX7313_PINMODE_OUTPUT        1
#define MAX7313_PINMODE_INPUT         0

#define __max7313_get_regmask(port) (1<<(port%8))
#define __max7313_get_input_reg(port) ((port < 8) ? MAX7313_READ_IN_00_07 : MAX7313_READ_IN_08_15)
#define __max7313_get_output_reg(port) __max7313_output_registers[port/2]
#define __max7313_get_phase_reg(port, phase) \
    (phase ? ( (port<8)? MAX7313_BLINK_PHASE_1_00_07:MAX7313_BLINK_PHASE_1_08_15  ) : \
     ((port<8)? MAX7313_BLINK_PHASE_0_00_07:MAX7313_BLINK_PHASE_0_08_15))

static const uint8_t __max7313_output_registers[9] = {
  MAX7313_OUT_INT_01_00,
  MAX7313_OUT_INT_03_02,
  MAX7313_OUT_INT_05_04,
  MAX7313_OUT_INT_07_06,
  MAX7313_OUT_INT_09_08,
  MAX7313_OUT_INT_11_10,
  MAX7313_OUT_INT_13_12,
  MAX7313_OUT_INT_15_14,
  MAX7313_OUT_INT_MA_16
};

void max7313_init(i2c_inst_t *i2c, uint8_t i2caddr, uint8_t sda, uint8_t scl);
void max7313_write8(i2c_inst_t *i2c, uint8_t i2caddr, uint8_t sda, uint8_t scl, uint8_t addr, uint8_t d);
uint8_t max7313_read8(i2c_inst_t *i2c, uint8_t i2caddr, uint8_t sda, uint8_t scl, uint8_t addr);
void max7313_enableInterrupt(i2c_inst_t *i2c, uint8_t i2caddr, uint8_t sda, uint8_t scl);
void max7313_disableInterrupt(i2c_inst_t *i2c, uint8_t i2caddr, uint8_t sda, uint8_t scl);
void max7313_clearInterrupt(i2c_inst_t *i2c, uint8_t i2caddr, uint8_t sda, uint8_t scl);
void max7313_analogWrite(i2c_inst_t *i2c, uint8_t i2caddr, uint8_t sda, uint8_t scl, uint8_t num, uint8_t val);
uint8_t max7313_digitalRead(i2c_inst_t *i2c, uint8_t i2caddr, uint8_t sda, uint8_t scl, uint8_t num);
void max7313_pinMode(i2c_inst_t *i2c, uint8_t i2caddr, uint8_t sda, uint8_t scl, uint8_t num, uint8_t mode);

#endif
