#ifndef GLOBALS_H_
#define GLOBALS_H_

AudioChain audioChain[NUM_CHANNELS];
AudioInputI2SQuad    audioInput; 
AudioOutputI2SQuad   audioOutput;
AudioControlSGTL5000 sgtl5000_1;
AudioControlSGTL5000 sgtl5000_2;

Menu menu;
DynamicJsonDocument preset(2048);
int currPreset = -1;

uint8_t volKnob = 0;
uint8_t cv[8] = {0, 0, 0, 0, 0, 0, 0, 0};
uint8_t cvPins[8] = {25, 26, 24, 22, 40, 17, 16, 41};
uint8_t audioIOMapping[4] = {1, 0, 3, 2};

#endif /* GLOBALS_H_ */
