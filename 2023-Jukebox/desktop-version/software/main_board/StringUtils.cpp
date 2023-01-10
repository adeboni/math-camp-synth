#include "StringUtils.h"
#include "Arduino.h"

int strEndsWith(const char* str, const char* suffix)
{
    if (!str || !suffix) return 0;
    size_t lenstr = strlen(str);
    size_t lensuffix = strlen(suffix);
    if (lensuffix > lenstr) return 0;
    return strncmp(str + lenstr - lensuffix, suffix, lensuffix) == 0;
}

void trimExtension(const char* str, char* buf) {
  for (int i = 0; i < strlen(str); i++) {
    if (str[i] == '.') {
      buf[i] = '\0';
      break;
    } else {
      buf[i] = str[i];
    }
  }
}

int percentStrToInt(const char* newVal) {
  char buf[16];
  strcpy(buf, newVal);
  buf[strlen(buf) - 1] = '\0';
  return atoi(buf);
}
