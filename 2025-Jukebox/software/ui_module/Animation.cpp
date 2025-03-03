#include "Animation.h"
#include "MAX7313.h"

Animation::Animation(MAX7313 *io1, MAX7313 *io2, uint8_t *dots, uint8_t *motors, uint8_t *lamp, uint8_t *mouth) {
  _io1 = io1;
  _io2 = io2;
  _dots = dots;
  _motors = motors;
  _lamp = lamp;
  _mouth = mouth;
}

void Animation::start() {
  running = true;
  nextUpdateTime = millis() + interval;
}

void Animation::stop() {
  running = false;
}

bool Animation::isRunning() {
  return running;
}

void Animation::setInterval(unsigned long t) {
  interval = t;
}

void Animation::setLength(unsigned long t) {
  length = t;
}

void Animation::reset() {
  for (int i = 0; i < 1; i++)  _io1->analogWrite(_lamp[i], 0);
  for (int i = 0; i < 15; i++) _io1->analogWrite(_mouth[i], 0);
  for (int i = 0; i < 3; i++)  _io2->analogWrite(_motors[i], 0);
  for (int i = 0; i < 6; i++)  _io2->analogWrite(_dots[i], 15);
}

void Animation::update() {
  static unsigned long lastUpdate = 0;
  static bool resetComplete = false;
  if (!running) return;

  unsigned long currTime = millis();
  if (currTime - lastUpdate < 20) return;
  lastUpdate = currTime;

  bool animationInProgress = startTime < currTime && startTime + length > currTime;
  bool animationDone = startTime + length < currTime;
  bool readyToStartAnimation = nextUpdateTime < currTime;

  if (readyToStartAnimation) {
    nextUpdateTime = currTime + length + interval;
    startTime = currTime;
    resetComplete = false;
    if (nextMode != -1) {
      currentMode = nextMode;
      nextMode = -1;
    } else {
      currentMode = (currentMode + 1) % NUM_MODES;
    }
  } else if (animationDone) {
    if (!resetComplete) {
      reset();
      resetComplete = true;
    }
    return;
  } else if (!animationInProgress) {
    return;
  }

  switch (currentMode) {
    case 0:
      dots_nightrider();
      break;
    case 1:
      mouth_pulse();
      break;
    case 2:
      motors_spin();
      break;
    case 3:
      lamp_morse_code();
      break;
    default:
      break;
  }
}

void Animation::forceAnimation(int mode) {
  nextMode = mode;
  nextUpdateTime = millis();
  reset();
}

uint8_t Animation::cube(double x) {
  int y = (int)(x * x * x / 15.0 / 15.0);
  if      (y > 15) return 15;
  else if (y < 0)  return 0;
  else             return y;
}

void Animation::dots_nightrider() {
  for (int i = 0; i < 6; i++) {
    double dotIndex = (sin((double)millis() / 1000.0 * 4.0) + 1.0) * 2.5;
    _io2->analogWrite(_dots[i], 15 - cube(15 - 3 * abs(i - dotIndex)));
  }
}

void Animation::mouth_pulse() {
  static int colorOffset = 0;
  static unsigned long lastColorUpdate = 0;
  static unsigned long lastIntensityUpdate = 0;

  if (millis() - lastColorUpdate > 10000) {
    lastColorUpdate = millis();
    colorOffset += 5;
    if (colorOffset > 10) colorOffset = 0;

    for (int i = 0; i < 15; i++)
      _io1->analogWrite(_mouth[i], 0);
  }

  if (millis() - lastIntensityUpdate > 100) {
    lastIntensityUpdate = millis();

    _io1->analogWrite(_mouth[0 + colorOffset], (uint8_t)random(6, 16));
    _io1->analogWrite(_mouth[1 + colorOffset], (uint8_t)random(0, 6));
    _io1->analogWrite(_mouth[2 + colorOffset], (uint8_t)random(0, 2));
    _io1->analogWrite(_mouth[3 + colorOffset], (uint8_t)random(0, 6));
    _io1->analogWrite(_mouth[4 + colorOffset], (uint8_t)random(6, 16));
  }
}

void Animation::motors_spin() {
  static uint8_t motorPattern[3] = {0, 0, 0};
  static unsigned long lastUpdate = 0;

  if (millis() - lastUpdate > 3000) {
    for (int i = 0; i < 3; i++)
      motorPattern[i] = random(2) * 4;
    lastUpdate = millis();
  }

  for (int i = 0; i < 3; i++)
    _io2->analogWrite(_motors[i], motorPattern[i]);
}

int Animation::expandMorseCode(int phraseIndex, uint8_t *phrase) {
  int len = 0;
  for (int i = 0; i < strlen(morse_phrases[phraseIndex]); i++) {
    int c = morse_phrases[phraseIndex][i] - 65;
    if (c == -33) {
      phrase[len++] = 0; phrase[len++] = 0; phrase[len++] = 0; phrase[len++] = 0;
    } else if (c >= 0 && c < 26) {
      for (int j = 0; j < strlen(morse_lut[c]); j++) {
        if (morse_lut[c][j] == '.') {
          phrase[len++] = 15;
        } else {
          phrase[len++] = 15; phrase[len++] = 15; phrase[len++] = 15;
        }
        phrase[len++] = 0; phrase[len++] = 0; phrase[len++] = 0;
      }
    }
  }
  phrase[len++] = 0; phrase[len++] = 0; phrase[len++] = 0; phrase[len++] = 0;
  phrase[len++] = 0; phrase[len++] = 0; phrase[len++] = 0;
  return len;
}

void Animation::lamp_morse_code() {
  static int currentPhraseIndex = 0;
  static unsigned long lastStartTime = 0;
  static int phraseLen = 0;
  static uint8_t phrase[1024];

  unsigned long phraseTimeIndex = (millis() - lastStartTime) / MORSE_TIME_UNIT_MS;
  if (phraseTimeIndex < phraseLen) {
    _io1->analogWrite(_lamp[0], phrase[phraseTimeIndex]);
  } else {
    lastStartTime = millis();
    currentPhraseIndex = (currentPhraseIndex + 1) % NUM_MORSE_PHRASES;
    phraseLen = expandMorseCode(currentPhraseIndex, phrase);
  }
}
