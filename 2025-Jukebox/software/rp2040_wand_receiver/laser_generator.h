#ifndef _LASER_GENERATOR_
#define _LASER_GENERATOR_

#include <Arduino.h>
#include "primitives.h"
#include "sierpinski.h"

class LaserGenerator {
  public:
    void init();
    void point_to_bytes(laser_point_t *p, uint8_t *buf, uint16_t i);
    laser_point_x3_t get_point(uint8_t mode);

  private:
    Sierpinski sier;

    laser_point_x3_t get_circle_point();
    laser_point_x3_t get_equation_point();
    laser_point_x3_t get_spirograph_point();
    laser_point_x3_t get_audio_visualizer_point();
    laser_point_x3_t get_pong_point();
    laser_point_x3_t get_drums_graphics_point();
    laser_point_x3_t get_wand_drawing_point();
    laser_point_x3_t get_calibration_point();
};

#endif