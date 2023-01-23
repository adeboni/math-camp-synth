#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

AudioInputI2SQuad    i2s_quad1;
AudioOutputI2SQuad   i2s_quad2;
AudioConnection      patchCord1(i2s_quad1, 0, i2s_quad2, 0);
AudioConnection      patchCord2(i2s_quad1, 1, i2s_quad2, 1);
AudioConnection      patchCord3(i2s_quad1, 2, i2s_quad2, 2);
AudioConnection      patchCord4(i2s_quad1, 3, i2s_quad2, 3);
AudioControlSGTL5000 sgtl5000_1;
AudioControlSGTL5000 sgtl5000_2;

void setup() {
  AudioMemory(50);

  AudioNoInterrupts();
  sgtl5000_1.setAddress(HIGH);
  sgtl5000_1.enable();
  sgtl5000_1.inputSelect(AUDIO_INPUT_LINEIN);
  sgtl5000_1.adcHighPassFilterDisable();
  sgtl5000_1.lineInLevel(5);

  sgtl5000_2.setAddress(LOW);
  sgtl5000_2.enable();
  sgtl5000_2.inputSelect(AUDIO_INPUT_LINEIN);
  sgtl5000_2.adcHighPassFilterDisable();
  sgtl5000_2.lineInLevel(5);
  AudioInterrupts();
}

void loop() {

}
