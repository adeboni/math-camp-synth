#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <RotaryEncoder.h>
#include <Bounce.h>
#include "StringUtils.h"

#define UP_BTN     29
#define DOWN_BTN   30
#define SELECT_BTN 28

#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 64

char wavFileName[30];
bool wavPlaying = false;

int numAudioFiles = 0;
char fileList[30][64];
int hoverRow = 0;
int selectedRow = 0;

AudioPlaySdRaw       playSdWav;
AudioInputI2SQuad    i2s_in;
AudioOutputI2SQuad   i2s_out;
AudioMixer4          i2s_mixer;
AudioMixer4          out_mixer;
AudioEffectReverb    reverb;
AudioConnection      patchCord01(i2s_in, 0, i2s_mixer, 0);
AudioConnection      patchCord02(i2s_in, 1, i2s_mixer, 1);
AudioConnection      patchCord03(i2s_in, 2, i2s_mixer, 2);
AudioConnection      patchCord04(i2s_in, 3, i2s_mixer, 3);
AudioConnection      patchCord05(i2s_mixer, 0, out_mixer, 0);
AudioConnection      patchCord06(playSdWav, 0, out_mixer, 1);
AudioConnection      patchCord07(out_mixer, 0, i2s_out, 0);
AudioConnection      patchCord08(out_mixer, 0, i2s_out, 1);
AudioConnection      patchCord09(out_mixer, 0, i2s_out, 2);
AudioConnection      patchCord10(out_mixer, 0, i2s_out, 3);
AudioControlSGTL5000 sgtl5000_1;
AudioControlSGTL5000 sgtl5000_2;

Adafruit_SH1106G display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
RotaryEncoder encoder(UP_BTN, DOWN_BTN, RotaryEncoder::LatchMode::TWO03);
Bounce button(SELECT_BTN, 10);

void setAudioInput(int input) {
  if (input == 0) {
    stopWavFiles();
    out_mixer.gain(0, 1);
    out_mixer.gain(1, 0);
    i2s_mixer.gain(0, 1);
    i2s_mixer.gain(1, 1);
    i2s_mixer.gain(2, 0);
    i2s_mixer.gain(3, 0);
  } else if (input == 1) {
    stopWavFiles();
    out_mixer.gain(0, 1);
    out_mixer.gain(1, 0);
    i2s_mixer.gain(0, 0);
    i2s_mixer.gain(1, 0);
    i2s_mixer.gain(2, 1);
    i2s_mixer.gain(3, 1);
  } else {
    out_mixer.gain(0, 0);
    out_mixer.gain(1, 1);
    i2s_mixer.gain(0, 0);
    i2s_mixer.gain(1, 0);
    i2s_mixer.gain(2, 0);
    i2s_mixer.gain(3, 0);

    sprintf(wavFileName, "/Audio/%s.raw", fileList[input]);
    playSdWav.play(wavFileName);
  }
}

void displayMessage(const char* msg) {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.print(msg);
  display.display();
}

void stopWavFiles() {
  wavPlaying = false;
  playSdWav.stop();
}

void resumeWavFiles() {
  if (wavPlaying && !playSdWav.isPlaying())
    playSdWav.play(wavFileName);
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

  strcpy(fileList[numAudioFiles++], "EXT 1");
  strcpy(fileList[numAudioFiles++], "EXT 2");
  File audioRoot = SD.open("/Audio/");
  while (true) {
     File entry = audioRoot.openNextFile();
     if (!entry) break;
     if (strEndsWith(entry.name(), ".raw")) {
        char buf[16];
        trimExtension(entry.name(), buf);
        strcpy(fileList[numAudioFiles++], buf);
     }
     entry.close();
  }
  audioRoot.close();

  displayMessage(fileList[0]);

  AudioNoInterrupts();
  sgtl5000_1.setAddress(HIGH);
  sgtl5000_1.enable();
  sgtl5000_1.inputSelect(AUDIO_INPUT_LINEIN);
  sgtl5000_1.adcHighPassFilterDisable();
  sgtl5000_1.lineInLevel(5);
  sgtl5000_1.dacVolumeRamp();
  sgtl5000_1.dacVolume(1);

  sgtl5000_2.setAddress(LOW);
  sgtl5000_2.enable();
  sgtl5000_2.inputSelect(AUDIO_INPUT_LINEIN);
  sgtl5000_2.adcHighPassFilterDisable();
  sgtl5000_2.lineInLevel(5);
  sgtl5000_2.dacVolumeRamp();
  sgtl5000_2.dacVolume(1);
  AudioInterrupts();
}

void updateMenu() {
  int i = 0;
  static int pos = 0;
  encoder.tick();
  int newPos = encoder.getPosition();
  if (pos != newPos) {
    i = (int)(encoder.getDirection());
    pos = newPos;
  }

  if (i != 0) 
    hoverRow = constrain(hoverRow + i, 0, numAudioFiles - 1);
  
  if (button.update() && button.fallingEdge()) {
    selectedRow = hoverRow;
    setAudioInput(selectedRow);
  }
    
  display.clearDisplay();
  display.setCursor(0, 0);

  int startPos = 0;
  if (hoverRow > 4) 
    startPos = hoverRow - 4;
  
  for (int i = startPos; i < min(startPos + 6, numAudioFiles); i++) {
    display.setCursor(0, (i - startPos) * 10);
    display.print(i == hoverRow ? '>' : ' ');
    if (selectedRow == i)
      display.setTextColor(SH110X_BLACK, SH110X_WHITE);
    display.print(fileList[i]);
    display.setTextColor(SH110X_WHITE);
  }
  display.display();
}

void loop() {
  int vol = map(analogRead(27), 0, 1023, 0, 100);
  sgtl5000_1.dacVolume((float)vol / 100);
  sgtl5000_2.dacVolume((float)vol / 100);

  resumeWavFiles();
  updateMenu();
}
