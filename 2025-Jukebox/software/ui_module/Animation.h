#ifndef _ANIMATION_H
#define _ANIMATION_H

#include "MAX7313.h"

//Animation modes
#define NUM_MODES          4
#define AM_DOTS_NIGHTRIDER 0
#define AM_MOUTH_PULSE     1
#define AM_MOTORS_SPIN     2
#define AM_LAMP_MORSE_CODE 3

class Animation {
  public:
    Animation(MAX7313 *io1, MAX7313 *io2, uint8_t *dots, uint8_t *motors, uint8_t *lamp, uint8_t *mouth);
    void start();
    void stop();
    bool isRunning();
    void setInterval(unsigned long t);
    void setLength(unsigned long t);
    void update();
    void forceAnimation(int mode);

  private:
    MAX7313 *_io1;
    MAX7313 *_io2;
    uint8_t *_dots;
    uint8_t *_motors;
    uint8_t *_lamp;
    uint8_t *_mouth;

    bool running = false;
    unsigned long interval = 30000;
    unsigned long length = 10000;
    unsigned long nextUpdateTime = 0;
    unsigned long startTime = 0;
    int currentMode = -1;
    int nextMode = -1;

    uint8_t cube(double x);
    void dots_nightrider();
    void mouth_pulse();
    void motors_spin();
    void lamp_morse_code();  
};

#endif
