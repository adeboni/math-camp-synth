#pragma once

#include <AudioOutputI2S.h>

#define UDP_AUDIO_BUFF_SIZE 1024

class AudioOutputI2SExtra : public AudioOutputI2S
{
    public: 
        uint8_t udpBuffer[UDP_AUDIO_BUFF_SIZE];
        virtual bool ConsumeSample(int16_t sample[2]) override;
        
    private:
        uint16_t udpBufferIndex = 0;
};