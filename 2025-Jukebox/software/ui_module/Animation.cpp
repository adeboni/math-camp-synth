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

void Animation::update() {
  if (!running) return;

  unsigned long currTime = millis();
  bool animationInProgress = startTime + length < currTime;
  bool readyToStartAnimation = nextUpdateTime < currTime;

  if (readyToStartAnimation) {
    nextUpdateTime = currTime + length + interval;
    startTime = currTime;
    if (nextMode != -1) {
      currentMode = nextMode;
      nextMode = -1;
    } else {
      currentMode = (currentMode + 1) % NUM_MODES;
    }
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
}

uint8_t Animation::cube(double x) {
  int y = (int)(x * x * x / 255.0 / 255.0);
  if      (y > 255) return 255;
  else if (y < 0)   return 0;
  else              return y;
}

void Animation::dots_nightrider() {
  for (int i = 0; i < 6; i++) {
    double dotIndex = sin((double)millis() / 1000.0 * 4.0) * 5.0 + 2.5;
    _io2->analogWrite(_dots[i], cube(255 - 51 * abs(i - dotIndex)) >> 4);
  }
}

void Animation::mouth_pulse() {
  double mouthColorIndex = sin((double)millis() / 1000.0 * 50.0 * 3.14 / 180.0) + 1.0;
  double mouthRedLevel = cube(255 - 85 * abs(0 - mouthColorIndex)) / 255.0 * 8.0;
  double mouthWhiteLevel = cube(255 - 85 * abs(1 - mouthColorIndex)) / 255.0 * 8.0;
  double mouthBlueLevel = cube(255 - 85 * abs(2 - mouthColorIndex)) / 255.0 * 8.0;
  for (int i = 0; i < 5; i++) {
    _io1->analogWrite(_mouth[i], (uint8_t)((sin((double)millis() / 1000.0 * 4.0) + 1) * mouthRedLevel));
    _io1->analogWrite(_mouth[i + 5], (uint8_t)((sin((double)millis() / 1000.0 * 4.0) + 1) * mouthWhiteLevel));
    _io1->analogWrite(_mouth[i + 10], (uint8_t)((sin((double)millis() / 1000.0 * 4.0) + 1) * mouthBlueLevel));
  }
}

void Animation::motors_spin() {
  static uint8_t motorPattern[3] = {0, 0, 0};
  static unsigned long lastUpdate = 0;

  if (millis() - lastUpdate > 3000) {
    for (int i = 0; i < 3; i++)
      motorPattern[i] = random(2) * 15;
    lastUpdate = millis();
  }

  for (int i = 0; i < 3; i++)
    _io2->analogWrite(_motors[i], motorPattern[i]);
}

void Animation::lamp_morse_code() {
  //TODO
}