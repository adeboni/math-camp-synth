#ifndef _LASER_GENERATOR_
#define _LASER_GENERATOR_

#include <Arduino.h>
#include "primitives.h"
#include "sierpinski.h"

#define UDP_AUDIO_BUFF_SIZE 1024

#define NUM_CIRCLES 6
#define MAX_CIRCLE_RADIUS 200

typedef struct {
  double d_r, r;
  uint16_t x, y;
} circle_t;

class LaserGenerator {
  public:
    void init();
    void point_to_bytes(laser_point_t *p, uint8_t *buf, uint16_t i);
    laser_point_x3_t get_point(uint8_t mode);
    void calibrate_wand(uint16_t x, uint16_t y, uint16_t z, uint16_t w);
    uint8_t audioBuffer[UDP_AUDIO_BUFF_SIZE];
    uint8_t numWandsConnected = 0;
    uint16_t wandData[4] = {0, 0, 0, 1};

  private:
    Sierpinski sier;
    
    laser_point_x3_t get_circle_point();
    laser_point_x3_t get_equation_point();
    laser_point_x3_t get_spirograph_point();
    laser_point_x3_t get_audio_visualizer_point();
    laser_point_x3_t get_pong_point();
    laser_point_x3_t get_wand_drawing_point();
    laser_point_x3_t get_calibration_point();
};

#endif
