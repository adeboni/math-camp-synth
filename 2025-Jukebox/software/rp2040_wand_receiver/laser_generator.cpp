#include "laser_generator.h"
#include "laser_objects.h"
#include "sierpinski.h"

void LaserGenerator::init() {
  sier.init();
}

void LaserGenerator::point_to_bytes(laser_point_t *p, uint8_t *buf, uint16_t i) {
  buf[i] = (p->x >> 4) & 0xff;
  buf[i + 1] = ((p->x & 0x0f) << 4) | ((p->y >> 8) & 0x0f);
  buf[i + 2] = p->y & 0xff;
  buf[i + 3] = p->r;
  buf[i + 4] = p->g;
  buf[i + 5] = p->b;
}

laser_point_x3_t LaserGenerator::get_point(uint8_t mode) { 
  switch (mode) {
    case 1:
      return get_circle_point(); // remove later
    case 2:
      return get_audio_visualizer_point();
    case 3:
      return get_equation_point();
    case 4:
      return get_spirograph_point();
    case 5:
      return get_pong_point();
    case 7:
    case 8:
      return get_wand_drawing_point();
    case 9:
      return get_calibration_point();
  }

  laser_point_x3_t empty;
  memset(&empty, 0, sizeof(laser_point_x3_t));
  return empty;
}

laser_point_x3_t LaserGenerator::get_circle_point() {
  static int angle = 0;

  angle = (angle + 1) % 360;
  uint16_t x = (uint16_t)(sin(angle * (3.14159 / 180.0)) * 200 + 2048);
  uint16_t y = (uint16_t)(cos(angle * (3.14159 / 180.0)) * 200 + 2048);

  uint8_t r = 0;
  uint8_t g = 0;
  uint8_t b = 0;

  if (angle < 120)
    r = 255;
  else if (angle < 240)
    g = 255;
  else
    b = 255;

  laser_point_t lp = (laser_point_t){x, y, r, g, b};
  return (laser_point_x3_t){lp, lp, lp};
}

laser_point_x3_t LaserGenerator::get_equation_point() {
  laser_point_x3_t empty;
  memset(&empty, 0, sizeof(laser_point_x3_t));
  return empty;
}

laser_point_x3_t LaserGenerator::get_spirograph_point() {
  laser_point_x3_t empty;
  memset(&empty, 0, sizeof(laser_point_x3_t));
  return empty;
}

laser_point_x3_t LaserGenerator::get_audio_visualizer_point() {
  laser_point_x3_t empty;
  memset(&empty, 0, sizeof(laser_point_x3_t));
  return empty;
}

laser_point_x3_t LaserGenerator::get_pong_point() {
  laser_point_x3_t empty;
  memset(&empty, 0, sizeof(laser_point_x3_t));
  return empty;
}

laser_point_x3_t LaserGenerator::get_drums_graphics_point() {
  laser_point_x3_t empty;
  memset(&empty, 0, sizeof(laser_point_x3_t));
  return empty;
}

laser_point_x3_t LaserGenerator::get_wand_drawing_point() {
  laser_point_x3_t empty;
  memset(&empty, 0, sizeof(laser_point_x3_t));
  return empty;
}

laser_point_x3_t LaserGenerator::get_calibration_point() {
  static int setup_complete = 0;
  static xy_t raw_bounds[5];
  static xy_t *interp_bounds;
  static int interp_bounds_size;
  static int curr_index = 0;
  if (setup_complete == 0) {
    sier.get_laser_coordinate_bounds(raw_bounds);
    raw_bounds[4] = raw_bounds[0];
    interp_bounds_size = get_interpolated_size(raw_bounds, 5, 8);
    interp_bounds = (xy_t*)malloc(interp_bounds_size * sizeof(xy_t));
    interpolate_objects(raw_bounds, 5, 8, interp_bounds);
    setup_complete = 1;
  }

  curr_index = (curr_index + 1) % interp_bounds_size;
  laser_point_t lp = (laser_point_t){interp_bounds[curr_index].x, interp_bounds[curr_index].y, 0, 255, 0};
  return (laser_point_x3_t){lp, lp, lp};
}
