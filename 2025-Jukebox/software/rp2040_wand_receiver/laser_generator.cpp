#include "laser_generator.h"
#include "laser_objects.h"
#include "sierpinski.h"
#include "spirograph.h"

void LaserGenerator::init() {
  sier.init();
  memset(audioBuffer, 0, UDP_AUDIO_BUFF_SIZE);
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
  static int audioBufIndex = 0;

  angle = (angle + 1) % 360;
  audioBufIndex = (audioBufIndex + 1) % UDP_AUDIO_BUFF_SIZE;
  double radius = 200 + (double)audioBuffer[audioBufIndex] / 2.5;
  uint16_t x = (uint16_t)(sin(angle * (3.14159 / 180.0)) * radius + 2048);
  uint16_t y = (uint16_t)(cos(angle * (3.14159 / 180.0)) * radius + 2048);

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

laser_point_x3_t LaserGenerator::get_audio_visualizer_point() {
  static int setup_complete = 0;
  static uint16_t bounds[4];

  if (setup_complete == 0) {
    sier.get_laser_rect_interior(bounds);
    setup_complete = 1;
  }

  laser_point_x3_t empty;
  memset(&empty, 0, sizeof(laser_point_x3_t));
  return empty;
}

laser_point_x3_t LaserGenerator::get_equation_point() {
  static int setup_complete = 0;
  static uint16_t bounds[4];

  if (setup_complete == 0) {
    sier.get_laser_rect_interior(bounds);
    setup_complete = 1;
  }

  laser_point_x3_t empty;
  memset(&empty, 0, sizeof(laser_point_x3_t));
  return empty;
}

laser_point_x3_t LaserGenerator::get_spirograph_point() {
  static int setup_complete = 0;
  static uint16_t bounds[4];
  static Spirograph spiro[3] = { Spirograph(), Spirograph(), Spirograph() };
  static uint16_t offsets[3][2];
  static int dirs[3][2];
  static double colors[3];
  static int iteration = 0;
  static bool pointMode = true;
  static unsigned long nextUpdate = 0;

  if (setup_complete == 0) {
    sier.get_laser_rect_interior(bounds);

    for (int i = 0; i < 3; i++) {
      spiro[i].init(random(100, 111), random(40, 81), random(3, 10) / 10.0, random(15, 26) / 100.0);
      spiro[i].add_delta((spiro_delta_t){DELTA_TYPE_R2, 0.00000002, 40.0, 80.0});
      spiro[i].add_delta((spiro_delta_t){DELTA_TYPE_A, 0.000000002, 0.3, 0.9});

      offsets[i][0] = (bounds[0] + bounds[1]) / 2;
      offsets[i][1] = (bounds[2] + bounds[3]) / 2;
      dirs[i][0] = random(2) ? 0.01 : -0.01;
      dirs[i][1] = random(2) ? 0.01 : -0.01;
      colors[i] = random(10) / 10.0;
    }
    
    setup_complete = 1;
  }

  if (millis() > nextUpdate) {
    pointMode = !pointMode;
    nextUpdate = millis() + 30000;
  }

  iteration++;
  laser_point_x3_t points;

  for (int i = 0; i < 3; i++) {
    rgb_t c = hsv_to_rgb(colors[i], 1.0, 1.0);
    colors[i] += 0.00001;
    if (colors[i] > 1) colors[i] = 0;

    if (!pointMode) {
      spiro[i].update(1.5, 1.5, offsets[i][0], offsets[i][1]);
      points.p[i] = (laser_point_t){(uint16_t)spiro[i].x, (uint16_t)spiro[i].y, c.r, c.g, c.b};
    } else if (iteration % 3 == 0) {
      points.p[i] = (laser_point_t){(uint16_t)spiro[i].x, (uint16_t)spiro[i].y, 0, 0, 0};
    } else if (iteration % 3 == 1) {
      points.p[i] = (laser_point_t){(uint16_t)spiro[i].x, (uint16_t)spiro[i].y, c.r, c.g, c.b};
    } else {
      spiro[i].update(1.5, 1.5, offsets[i][0], offsets[i][1]);
      points.p[i] = (laser_point_t){(uint16_t)spiro[i].x, (uint16_t)spiro[i].y, 0, 0, 0};
    }

    offsets[i][0] += dirs[i][0];
    offsets[i][1] += dirs[i][1];
    if      (spiro[i].x < bounds[0] && dirs[i][0] < 0) dirs[i][0] *= -1;
    else if (spiro[i].x > bounds[1] && dirs[i][0] > 0) dirs[i][0] *= -1;
    if      (spiro[i].y < bounds[2] && dirs[i][1] < 0) dirs[i][1] *= -1;
    else if (spiro[i].y > bounds[3] && dirs[i][1] > 0) dirs[i][1] *= -1;
  }

  return points;
}

laser_point_x3_t LaserGenerator::get_pong_point() {
  laser_point_x3_t empty;
  memset(&empty, 0, sizeof(laser_point_x3_t));
  return empty;
}

laser_point_x3_t LaserGenerator::get_wand_drawing_point() {
  static int setup_complete = 0;
  static uint16_t bounds[4];

  if (setup_complete == 0) {
    sier.get_laser_rect_interior(bounds);
    setup_complete = 1;
  }

  laser_point_x3_t points;
  memset(&points, 0, sizeof(laser_point_x3_t));
  if (numWandsConnected == 0) return points;

  double q[4] = {
    ((double)wandData[0] - 16384) / 16384,
    ((double)wandData[1] - 16384) / 16384,
    ((double)wandData[2] - 16384) / 16384,
    ((double)wandData[3] - 16384) / 16384
  };

  int laserIndex = -1;
  double v[3];
  sier.get_wand_projection(q, &laserIndex, v);
  if (laserIndex < 0) return points;

  xy_t lp = sier.sierpinski_to_laser_coords(laserIndex, v);
  points.p[laserIndex].x = lp.x;
  points.p[laserIndex].y = lp.y;
  points.p[laserIndex].r = 255;
  return points;
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

void LaserGenerator::calibrate_wand(uint16_t x, uint16_t y, uint16_t z, uint16_t w) {
  double q[4] = {
    ((double)x - 16384) / 16384,
    ((double)y - 16384) / 16384,
    ((double)z - 16384) / 16384,
    ((double)w - 16384) / 16384
  };
  sier.calibrate_wand_position(q);
}