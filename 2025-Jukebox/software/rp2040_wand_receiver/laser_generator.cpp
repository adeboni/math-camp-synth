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
  double radius = 200.0 + ((double)audioBuffer[audioBufIndex] - 128) * 40.0;
  uint16_t x = (uint16_t)(cos(angle * 3.14159 / 180.0) * radius + 2048);
  uint16_t y = (uint16_t)(sin(angle * 3.14159 / 180.0) * radius + 2048);

  uint8_t r = 0;
  uint8_t g = 0;
  uint8_t b = 0;

  if (angle < 120)      r = 255;
  else if (angle < 240) g = 255;
  else                  b = 255;

  laser_point_t lp = (laser_point_t){x, y, r, g, b};
  return (laser_point_x3_t){lp, lp, lp};
}

laser_point_x3_t LaserGenerator::get_audio_visualizer_point() {
  static int setup_complete = 0;
  static uint16_t bounds[4];
  static circle_t circles[3][NUM_CIRCLES];
  static int circleIndex = 0;
  static int angle = 0;
  static int audioBufIndex = 0;

  if (setup_complete == 0) {
    sier.get_laser_rect_interior(bounds);

    for (int i = 0; i < 3; i++) {
      for (int j = 0; j < NUM_CIRCLES; j++) {
        circles[i][j] = (circle_t){ 
          random(15, 40) / 10.0,
          0.0, 
          (uint16_t)random(bounds[0] + MAX_CIRCLE_RADIUS, bounds[1] - MAX_CIRCLE_RADIUS),
          (uint16_t)random(bounds[2] + MAX_CIRCLE_RADIUS, bounds[3] - MAX_CIRCLE_RADIUS)
        };
      }
    }

    setup_complete = 1;
  }

  audioBufIndex = (audioBufIndex + 1) % UDP_AUDIO_BUFF_SIZE;
  angle = (angle + 5) % 360;
  if (angle == 0) {
    circleIndex = (circleIndex + 1) % NUM_CIRCLES;
    if (circleIndex == 0) {
      for (int i = 0; i < 3; i++) {
        for (int j = 0; j < NUM_CIRCLES; j++) {
          if (circles[i][j].r >= MAX_CIRCLE_RADIUS && circles[i][j].d_r > 0) {
            circles[i][j].d_r *= -1;
          } else if (circles[i][j].r <= 0 && circles[i][j].d_r < 0) {
            circles[i][j].d_r = random(15, 40) / 10.0;
            circles[i][j].r = 0.0;
            circles[i][j].x = (uint16_t)random(bounds[0] + MAX_CIRCLE_RADIUS, bounds[1] - MAX_CIRCLE_RADIUS);
            circles[i][j].y = (uint16_t)random(bounds[2] + MAX_CIRCLE_RADIUS, bounds[3] - MAX_CIRCLE_RADIUS);
          }
          circles[i][j].r += circles[i][j].d_r;
        }
      }
    }
  }

  laser_point_x3_t points;
  memset(&points, 0, sizeof(laser_point_x3_t));

  for (int i = 0; i < 3; i++) {
    double x = cos(angle * 3.14159 / 180.0) * circles[i][circleIndex].r + circles[i][circleIndex].x;
    double y = sin(angle * 3.14159 / 180.0) * circles[i][circleIndex].r + circles[i][circleIndex].y;
    bool skip = false;
    for (int j = 0; j < NUM_CIRCLES; j++) {
      if (j == circleIndex) continue;
      if (sqrt((circles[i][j].x - x) * (circles[i][j].x - x) + (circles[i][j].y - y) * (circles[i][j].y - y)) <= circles[i][j].r) {
        skip = true;
        break;
      }
    }

    if (skip || angle == 0 || angle == 355)
      points.p[i] = (laser_point_t){(uint16_t)x, (uint16_t)y, 0, 0, 0};
    else
      points.p[i] = (laser_point_t){(uint16_t)x, (uint16_t)y, 0, 0, 255};
  }

  return points;
}

laser_point_x3_t LaserGenerator::get_equation_point() {
  static int setup_complete = 0;
  static uint16_t bounds[4];
  static unsigned long nextUpdate = 0;
  static int equationIndex[3] = {0, 1, 2};
  static int colorIndex[3] = {0, 1, 2};
  static int pointIndex[3] = {0, 0, 0};
  static uint16_t offsets[3][2];
  static int dirs[3][2];

  if (setup_complete == 0) {
    sier.get_laser_rect_interior(bounds);

    for (int i = 0; i < 3; i++) {
      offsets[i][0] = (bounds[0] + bounds[1]) / 2;
      offsets[i][1] = (bounds[2] + bounds[3]) / 2;
      dirs[i][0] = random(2) ? 2 : -2;
      dirs[i][1] = random(2) ? 2 : -2;
    }

    setup_complete = 1;
  }

  if (millis() > nextUpdate) {
    for (int i = 0; i < 3; i++) {
      equationIndex[i] = (equationIndex[i] + 3) % NUM_EQUATIONS;
      colorIndex[i] = (colorIndex[i] + 3) % 7;
      pointIndex[i] = 0;
    }
    nextUpdate = millis() + 30000;
  }

  laser_point_x3_t points;
  memset(&points, 0, sizeof(laser_point_x3_t));
  return points;
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

laser_point_x3_t LaserGenerator::get_pong_ball(uint8_t ballLaser, double ballX, double ballY, bool *done) {
  static int angle = 0;

  laser_point_x3_t points;
  memset(&points, 0, sizeof(laser_point_x3_t));

  uint16_t x = (uint16_t)(cos(angle * 3.14159 / 180.0) * PONG_BALL_RADIUS + ballX);
  uint16_t y = (uint16_t)(sin(angle * 3.14159 / 180.0) * PONG_BALL_RADIUS + ballY);
  uint8_t color = angle == 0 ? 0 : 255;
  points.p[ballLaser] = (laser_point_t){x, y, color, color, color};
  angle = (angle + 30) % 420;
  *done = (angle == 0);
  
  return points;
}

laser_point_x3_t LaserGenerator::get_pong_paddles(uint8_t ballLaser, double ballX, double ballY, 
                 double centerX, double centerY, double leftPaddle, double rightPaddle, bool *done) {
  static int setup_complete = 0;
  static xy_t paddlePoints[6];
  static int interpSize = 0;
  static xy_t *interpPoints;
  static int index = 0;
  
  if (setup_complete == 0) {
    if (ballLaser == 0) {
      paddlePoints[0] = (xy_t){(uint16_t)ballX, (uint16_t)ballY, 0};
      paddlePoints[1] = (xy_t){(uint16_t)(centerX - PONG_PADDLE_GAP), (uint16_t)(leftPaddle - PONG_PADDLE_HALF_HEIGHT), 0};
      paddlePoints[2] = (xy_t){(uint16_t)(centerX - PONG_PADDLE_GAP), (uint16_t)(leftPaddle + PONG_PADDLE_HALF_HEIGHT), 1};
      paddlePoints[3] = (xy_t){(uint16_t)(centerX + PONG_PADDLE_GAP), (uint16_t)(rightPaddle + PONG_PADDLE_HALF_HEIGHT), 0};
      paddlePoints[4] = (xy_t){(uint16_t)(centerX + PONG_PADDLE_GAP), (uint16_t)(rightPaddle - PONG_PADDLE_HALF_HEIGHT), 1};
      paddlePoints[5] = (xy_t){(uint16_t)ballX, (uint16_t)ballY, 0};
    } else {
      paddlePoints[0] = (xy_t){(uint16_t)(centerX + PONG_PADDLE_GAP), (uint16_t)(rightPaddle - PONG_PADDLE_HALF_HEIGHT), 0};
      paddlePoints[1] = (xy_t){(uint16_t)(centerX - PONG_PADDLE_GAP), (uint16_t)(leftPaddle - PONG_PADDLE_HALF_HEIGHT), 0};
      paddlePoints[2] = (xy_t){(uint16_t)(centerX - PONG_PADDLE_GAP), (uint16_t)(leftPaddle + PONG_PADDLE_HALF_HEIGHT), 1};
      paddlePoints[3] = (xy_t){(uint16_t)(centerX + PONG_PADDLE_GAP), (uint16_t)(rightPaddle + PONG_PADDLE_HALF_HEIGHT), 0};
      paddlePoints[4] = (xy_t){(uint16_t)(centerX + PONG_PADDLE_GAP), (uint16_t)(rightPaddle - PONG_PADDLE_HALF_HEIGHT), 1};
      paddlePoints[5] = (xy_t){(uint16_t)(centerX + PONG_PADDLE_GAP), (uint16_t)(rightPaddle - PONG_PADDLE_HALF_HEIGHT), 0};
    }

    int interpSize = get_interpolated_size(paddlePoints, 6, 8);
    interpPoints = new xy_t[interpSize];
    interpolate_objects(paddlePoints, 6, 8, interpPoints);
    setup_complete = 1;
  }

  laser_point_x3_t points;
  memset(&points, 0, sizeof(laser_point_x3_t));
  uint8_t color = interpPoints[index].on == 0 ? 0 : 255;
  points.p[0] = (laser_point_t){interpPoints[index].x, interpPoints[index].y, color, color, color};
  index = (index + 1) % interpSize;
  if (index == 0) {
    setup_complete = 0;
    delete[] interpPoints;
    *done = true;
  } else {
    *done = false;
  }

  return points;
}

laser_point_x3_t LaserGenerator::get_pong_point() {
  static int setup_complete = 0;
  static uint16_t bounds[4];
  static double centerX, centerY;
  static uint8_t ballLaser = 1;
  static bool scoreTimeout = false;
  static unsigned long gameResetTime = 0;
  static double ballX, ballY;
  static double dx = PONG_START_SPEED;
  static double dy = PONG_START_SPEED;
  static double leftPaddle, rightPaddle;
  static int drawMode = 0;

  if (setup_complete == 0) {
    sier.get_laser_rect_interior(bounds);
    centerX = (bounds[0] + bounds[1]) / 2.0;
    centerY = (bounds[2] + bounds[3]) / 2.0;
    ballX = centerX;
    ballY = centerY;
    leftPaddle = centerY;
    rightPaddle = centerY;
    setup_complete = 1;
  }

  if (drawMode == 0) {
    bool done;
    laser_point_x3_t lp = get_pong_ball(ballLaser, ballX, ballY, &done);
    if (done) drawMode = 1;
    return lp;
  }

  if (drawMode == 1) {
    bool done;
    laser_point_x3_t lp = get_pong_paddles(ballLaser, ballX, ballY, centerX, centerX, leftPaddle, rightPaddle, &done);
    if (done) drawMode = 2;
    return lp;
  }

  if (scoreTimeout && millis() > gameResetTime) {
    scoreTimeout = false;
    ballX = centerX;
    ballY = centerY;
    ballLaser = 1;
    dx = PONG_START_SPEED;
    dy = PONG_START_SPEED;
  }

  if (!scoreTimeout) {
    ballX += dx;
    ballY += dy;

    if (ballX < bounds[0]) {
      ballLaser = (ballLaser + 3 - 1) % 3;
      ballX = bounds[1];
    } else if (ballX > bounds[1]) {
      ballLaser = (ballLaser + 1) % 3;
      ballX = bounds[0];
    }

    if ((ballY < bounds[2] && dy < 0) || (ballY > bounds[3] && dy > 0)) {
      dy *= -1;
      playSoundEffect = SOUND_EFFECT_PONG_WALL;
    }

    if (ballLaser == 0 && dx > 0 && ballX < centerX && ballX > (centerX - PONG_PADDLE_GAP)) {
      if (abs(ballY - leftPaddle) < PONG_PADDLE_HALF_HEIGHT) {
        dx *= -1;
        playSoundEffect = SOUND_EFFECT_PONG_PADDLE;
      } else {
        scoreTimeout = true;
        gameResetTime = millis() + 3000;
        playSoundEffect = SOUND_EFFECT_PONG_GAMEOVER;
      }
    } else if (ballLaser == 0 && dx < 0 && ballX > centerX && ballX < (centerX + PONG_PADDLE_GAP)) {
      if (abs(ballY - rightPaddle) < PONG_PADDLE_HALF_HEIGHT) {
        dx *= -1;
        playSoundEffect = SOUND_EFFECT_PONG_PADDLE;
      } else {
        scoreTimeout = true;
        gameResetTime = millis() + 3000;
        playSoundEffect = SOUND_EFFECT_PONG_GAMEOVER;
      }
    }

    if (numWandsConnected > 0) {
      int laserIndex = -1;
      double v[3];
      sier.get_wand_projection(wandData1, &laserIndex, v);
      if (laserIndex >= 0) {
        xy_t lp = sier.sierpinski_to_laser_coords(laserIndex, v);
        leftPaddle += max(min(lp.y, bounds[3] - PONG_PADDLE_HALF_HEIGHT), bounds[2] + PONG_PADDLE_HALF_HEIGHT) - leftPaddle;
      }
    } else {
      if (leftPaddle < ballY && leftPaddle + PONG_PADDLE_HALF_HEIGHT < bounds[3])
        leftPaddle += PONG_AI_SPEED;
      else if (leftPaddle > ballY && leftPaddle - PONG_PADDLE_HALF_HEIGHT > bounds[2])
        leftPaddle -= PONG_AI_SPEED;
    }

    if (numWandsConnected > 1) {
      int laserIndex = -1;
      double v[3];
      sier.get_wand_projection(wandData2, &laserIndex, v);
      if (laserIndex >= 0) {
        xy_t lp = sier.sierpinski_to_laser_coords(laserIndex, v);
        rightPaddle += max(min(lp.y, bounds[3] - PONG_PADDLE_HALF_HEIGHT), bounds[2] + PONG_PADDLE_HALF_HEIGHT) - rightPaddle;
      }
    } else {
      if (rightPaddle < ballY && rightPaddle + PONG_PADDLE_HALF_HEIGHT < bounds[3])
        rightPaddle += PONG_AI_SPEED;
      else if (rightPaddle > ballY && rightPaddle - PONG_PADDLE_HALF_HEIGHT > bounds[2])
        rightPaddle -= PONG_AI_SPEED;
    }
  }

  drawMode = 0;
  return get_pong_point();
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

  int laserIndex = -1;
  double v[3];
  sier.get_wand_projection(wandData1, &laserIndex, v);
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
    interp_bounds = new xy_t[interp_bounds_size];
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
