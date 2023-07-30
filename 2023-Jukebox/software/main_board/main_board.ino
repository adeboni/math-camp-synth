#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <RotaryEncoder.h>
#include <Bounce.h>
#include <SerialTransfer.h>
#include "StringUtils.h"

#define UP_BTN     29
#define DOWN_BTN   30
#define SELECT_BTN 28

#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 64

AudioPlaySdRaw       playSdWav;
AudioOutputI2SQuad   i2s_out;
AudioConnection      patchCord1(playSdWav, 0, i2s_out, 0);
AudioConnection      patchCord2(playSdWav, 0, i2s_out, 1);
AudioConnection      patchCord3(playSdWav, 0, i2s_out, 2);
AudioConnection      patchCord4(playSdWav, 0, i2s_out, 3);
AudioControlSGTL5000 sgtl5000_1;
AudioControlSGTL5000 sgtl5000_2;

Adafruit_SH1106G display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
RotaryEncoder encoder(UP_BTN, DOWN_BTN, RotaryEncoder::LatchMode::TWO03);
Bounce button(SELECT_BTN, 10);

struct {
  uint8_t letter;
  uint8_t number;
} rxStruct;

struct {
  uint32_t songLength;
  uint8_t songName[20];
} txStruct;

SerialTransfer uartTransfer;

void setAudioInput(uint8_t letter, uint8_t number) {
  playSdWav.stop();
  delay(500);
  displayMessage("Stopped");
  
  char wavFileName[16];
  char txtFileName[16];
  sprintf(wavFileName, "/Audio/%c/%c.RAW", (char)letter, (char)number);
  sprintf(txtFileName, "/Audio/%c/%c.TXT", (char)letter, (char)number);
  
  if (!SD.exists(wavFileName)) {
    return;
  }
  else {
    memset(txStruct.songName, 0, sizeof(txStruct.songName));
    File songInfo = SD.open(txtFileName);
    if (songInfo) {
      int bytesRead = 0;
      while (songInfo.available() && bytesRead < 19)
        txStruct.songName[bytesRead++] = songInfo.read();
      songInfo.close();
    }

    txStruct.songLength = (uint32_t)(playSdWav.lengthMillis());
    uartTransfer.sendDatum(txStruct);
    displayMessage(wavFileName);
    playSdWav.play(wavFileName);
  }
}

void displayMessage(const char* msg) {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.print(msg);
  display.display();
}

void setup() {
  AudioMemory(50);
  
  pinMode(SELECT_BTN, INPUT_PULLUP);
  display.begin(0x3C, true);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
  display.display();
  
  if (!SD.begin(BUILTIN_SDCARD)) {
    displayMessage("SD init error");
    while (true) ;
  }

  AudioNoInterrupts();
  sgtl5000_1.setAddress(HIGH);
  sgtl5000_1.enable();
  sgtl5000_1.dacVolumeRamp();
  sgtl5000_1.dacVolume(1);

  sgtl5000_2.setAddress(LOW);
  sgtl5000_2.enable();
  sgtl5000_2.dacVolumeRamp();
  sgtl5000_2.dacVolume(1);
  AudioInterrupts();

  Serial4.begin(9600);
  uartTransfer.begin(Serial4);

  displayMessage("Ready");
}

void loop() {
  int vol = map(analogRead(25), 0, 1023, 0, 100);
  sgtl5000_1.dacVolume((float)vol / 100);
  sgtl5000_2.dacVolume((float)vol / 100);

  if (uartTransfer.available()) {
    uartTransfer.rxObj(rxStruct);
    setAudioInput(rxStruct.letter, rxStruct.number);
  }
}
