#ifndef GLOBALS_H_
#define GLOBALS_H_

LEDChannel ledChannel[NUM_CHANNELS];

AudioChain audioChain[NUM_CHANNELS];
AudioInputI2SQuad    audioInput; 
AudioOutputI2SQuad   audioOutput;
AudioMixer4          mixer;
AudioConnection      patchCord1(audioInput, 0, mixer, 0);
AudioConnection      patchCord2(audioInput, 1, mixer, 1);
AudioControlSGTL5000 sgtl5000_1;
AudioControlSGTL5000 sgtl5000_2;

Menu menu;
DynamicJsonDocument preset(2048);
int currPreset = -1;

uint8_t vcoKnob = 0;
uint8_t vcfKnob = 0;
uint8_t vcaKnob = 0;
uint8_t volKnob = 0;
uint8_t xVal = 0;
uint8_t yVal = 0;
uint8_t angleVal = 0;
uint8_t radiusVal = 0;
uint8_t midiVal = 0;

#endif /* GLOBALS_H_ */
