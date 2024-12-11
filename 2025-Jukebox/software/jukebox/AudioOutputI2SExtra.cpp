#include "AudioOutputI2SExtra.h"

bool AudioOutputI2SExtra::ConsumeSample(int16_t sample[2]) {
    udpBuffer[udpBufferIndex] = (uint8_t)sample[0];
    udpBufferIndex = (udpBufferIndex + 1) % UDP_AUDIO_BUFF_SIZE;
    return AudioOutputI2S::ConsumeSample(sample);
}