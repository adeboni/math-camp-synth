#include "AudioFileSourceSDExtra.h"

int32_t AudioFileSourceSDExtra::position() {
  return f ? f.position() : 0;
}
