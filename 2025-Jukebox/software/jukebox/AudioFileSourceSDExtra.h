#pragma once

#include <AudioFileSourceSD.h>

class AudioFileSourceSDExtra : public AudioFileSourceSD
{
  public: 
    virtual int32_t position() override;
};
