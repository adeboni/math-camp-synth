#include "pico/stdlib.h"
#include "MAX7313.h"

void max7313_init(uint8_t i2caddr, uint8_t portConfLow, uint8_t portConfHigh) {
  max7313_write8(i2caddr, MAX7313_PORTS_CONF_00_07, portConfLow); // pinmode: 0 = OUTPUT / 1 = INPUT
  max7313_write8(i2caddr, MAX7313_PORTS_CONF_08_15, portConfHigh);
  max7313_write8(i2caddr, MAX7313_BLINK_PHASE_0_00_07, 0xff);
  max7313_write8(i2caddr, MAX7313_BLINK_PHASE_0_08_15, 0xff);
  max7313_write8(i2caddr, MAX7313_BLINK_PHASE_1_00_07, 0xff);
  max7313_write8(i2caddr, MAX7313_BLINK_PHASE_1_08_15, 0xff);
  max7313_write8(i2caddr, MAX7313_CONFIGURATION, 0x01); // enable blink phase for PWM
  max7313_write8(i2caddr, MAX7313_OUT_INT_MA_16, 0xff);
}

void max7313_analogWrite(uint8_t i2caddr, uint8_t num, uint8_t val) {
  if (val > 0xf)
    val = 0xf;

  // "[the LED] cannot be turned fully off using the PWM intensity control."
  // using Blink Mode to correct this issue
  // see: page 17: "Using PWM Intensity Controls with Blink Disabled"
  // see: page 17: "Using PWM Intensity Controls with Blink Enabled"
  // see: page 18: Table 10
  // see: page 20: Table 12 (Important !)
  // fliping the on-state in the phase 0 register to turn the port fully off
  uint8_t phase0 = max7313_read8(i2caddr, __max7313_get_phase_reg(num, 0));
  if (val == 0) {
    // Datasheet Table 12.
    val = 0xF;                     // turn on to max, because the inversion makes it completely off
    phase0 &= (~(1 << (num % 8))); // inverted on state
  } else {
    phase0 |= (1 << (num % 8)); // regular  on state
  }
  max7313_write8(i2caddr, __max7313_get_phase_reg(num, 0), phase0);

  // two ports share one 8 bit register
  // this is why the registers needs to be read without changing the bits
  // of the other port
  //
  // D7 D6 D5 D4 | D3 D2 D1 D0
  // (odd)       | (even)
  // P 1,3,5 ... | P 0,2,4 ...
  uint8_t intensity = max7313_read8(i2caddr, __max7313_get_output_reg(num));
  if (num % 2)                                            // odd  (1,3,5 ...)
    intensity = (intensity & 0x0F) + ((val & 0x0F) << 4); // leave low bits and shift new intensity val to high bits
  else                                                    // even (0,2,4 ...)
    intensity = (intensity & 0xF0) + (val & 0x0F);        // leave high bits
  max7313_write8(i2caddr, __max7313_get_output_reg(num), intensity);
}

void max7313_enableInterrupt(uint8_t i2caddr) {
  uint8_t conf = max7313_read8(i2caddr, MAX7313_CONFIGURATION);
  conf |= 0x08;
  max7313_write8(i2caddr, MAX7313_CONFIGURATION, conf);
  max7313_clearInterrupt(i2caddr);
}

void max7313_disableInterrupt(uint8_t i2caddr) {
  uint8_t conf = max7313_read8(i2caddr, MAX7313_CONFIGURATION);
  conf &= 0xF7; // clear bit 3
  max7313_write8(i2caddr, MAX7313_CONFIGURATION, conf);
  max7313_clearInterrupt(i2caddr);
}

void max7313_clearInterrupt(uint8_t i2caddr) {
  max7313_read8(i2caddr, MAX7313_READ_IN_00_07);
  max7313_read8(i2caddr, MAX7313_READ_IN_08_15);
}

uint8_t max7313_digitalRead(uint8_t i2caddr, uint8_t num) {
  // 16 pins total, stored in 2 separate 8 bit registers
  // the following macro reads the correct one
  uint8_t all_regs = max7313_read8(i2caddr, __max7313_get_input_reg(num));
  // convert pin num to bit position in 8 bit register (0->0, 1->1, 2->2, 3->4, 4->8 ... 8->0, 9->1, 10->2, 11->4 ...)
  num = 1 << (num % 8);
  all_regs &= num;                // mask with bit position
  return (uint8_t)(all_regs > 0); // convert and return boolean expression (if bit set)
}

uint8_t max7313_read8(uint8_t i2caddr, uint8_t addr) {
  uint8_t val;
  i2c_write_blocking(I2C_INST, i2caddr, &addr, 1, true);
  i2c_read_blocking(I2C_INST, i2caddr, &val, 1, false);
  return val;
}

void max7313_write8(uint8_t i2caddr, uint8_t addr, uint8_t d) {
  uint8_t buf[] = {addr, d};
  i2c_write_blocking(I2C_INST, i2caddr, buf, 2, false);
}
