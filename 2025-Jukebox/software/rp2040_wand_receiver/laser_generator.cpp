#include "laser_generator.h"
#include "sierpinski.h"
#include "spirograph.h"

void LaserGenerator::init() {
  sier.init();
  memset(audioBuffer, 128, UDP_AUDIO_BUFF_SIZE);
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
      break;
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

laser_point_x3_t LaserGenerator::get_audio_visualizer_point() {
  static int setup_complete = 0;
  static uint16_t bounds[4];
  static double centerX, centerY;

  static int audioBufIndex = 0;
  static double colorOffset = 0;

  static double circleX, circleY;
  static double dirX = 0.02; 
  static double dirY = 0.02;
  static int angle = 0;
  
  static double sinePosX;
  static double sineDirX = 10.0;
  static double sineDirY = 0.02;
  static double sinePeaks = 1.0;
  static double sineAmp = 0;

  static double ccRadius = 50;
  static double ccDir = 0.02;

  if (setup_complete == 0) {
    sier.get_laser_rect_interior(bounds);
    centerX = (bounds[0] + bounds[1]) / 2.0;
    centerY = (bounds[2] + bounds[3]) / 2.0;
    circleX = centerX;
    circleY = centerY;
    sinePosX = (double)bounds[0];
    setup_complete = 1;
  }

  laser_point_x3_t points;
  audioBufIndex = (audioBufIndex + 1) % UDP_AUDIO_BUFF_SIZE;
  double audioAmp = ((double)audioBuffer[audioBufIndex] - 128);
  colorOffset += 0.005;

  double r1 = audioAmp * 2.5 + 100.0;
  angle += 2;
  circleX += dirX;
  circleY += dirY;
  if      (circleX - abs(r1) < bounds[0] && dirX < 0) dirX *= -1;
  else if (circleX + abs(r1) > bounds[1] && dirX > 0) dirX *= -1;
  if      (circleY - abs(r1) < bounds[2] && dirY < 0) dirY *= -1;
  else if (circleY + abs(r1) > bounds[3] && dirY > 0) dirY *= -1;

  uint16_t cx = (uint16_t)(cos(angle * PI / 180.0) * r1 + circleX);
  uint16_t cy = (uint16_t)(sin(angle * PI / 180.0) * r1 + circleY);
  rgb_t c1 = sier.get_color_from_angle((int)(angle * 0.005 + colorOffset));
  points.p[0] = (laser_point_t) { cx, cy, c1.r, c1.g, c1.b };


  sinePosX += sineDirX;
  if      (sinePosX < bounds[0] && sineDirX < 0) sineDirX *= -1;
  else if (sinePosX > bounds[1] && sineDirX > 0) sineDirX *= -1;

  double r2 = audioAmp * 5.0;
  double prevSineAmp = sineAmp;
  sineAmp += sineDirY;
  if      (centerY - abs(sineAmp + r2) < bounds[2] && sineDirY < 0) sineDirY *= -1;
  else if (centerY + abs(sineAmp + r2) > bounds[3] && sineDirY > 0) sineDirY *= -1;
  if (prevSineAmp > 0 && sineAmp < 0) {
    sinePeaks += 0.5;
    if (sinePeaks > 2) sinePeaks = 1;
  }
  
  uint16_t sineY = (uint16_t)(sin(TWO_PI / (bounds[3] - bounds[2]) * sinePeaks * sinePosX) * (sineAmp + r2) + centerY);
  rgb_t c2 = sier.get_color_from_angle((int)(sinePosX * 0.1 + colorOffset));
  points.p[1] = (laser_point_t) { (uint16_t)sinePosX, sineY, c2.r, c2.g, c2.b };


  ccRadius += ccDir;
  if (ccRadius < 100 && ccDir < 0) ccDir *= -1;
  else if (ccRadius > 200 && ccDir > 0) ccDir *= -1;
  double r3 = audioAmp * ccRadius / 40.0 + ccRadius;
  uint16_t ccx = (uint16_t)(cos(angle * PI / 180.0) * r3 + centerX);
  uint16_t ccy = (uint16_t)(sin(angle * PI / 180.0) * r3 + centerY);
  points.p[2] = (laser_point_t) { ccx, ccy, c1.r, c1.g, c1.b };

  return points;
}

int LaserGenerator::setup_equation(int index, xy_t *result, int *size) {
  static xy_t temp[500];
  if (EQUATION_LENS[index] / 2 + 1 > 500) return -1;
  int len = convert_to_xy(EQUATION_LIST[index], EQUATION_LENS[index], 0.5, 0.5, temp);
  get_laser_obj_size(temp, len, &size[0], &size[1]);

  int interpLen = get_interpolated_size(temp, len, 8);
  if (interpLen > 2500) return -1;
  interpolate_objects(temp, len, 8, result);
  return interpLen;
}

laser_point_x3_t LaserGenerator::get_equation_point() {
  static int setup_complete = 0;
  static uint16_t bounds[4];
  static unsigned long nextUpdate = 0;
  static int equationIndex[3] = {0, 1, 2};
  static int colorIndex[3] = {0, 1, 2};
  static int pointIndex[3] = {0, 0, 0};
  static int offsets[3][2];
  static int dirs[3][2];
  static int eqSize[3][2];
  static xy_t equations[3][2500];
  static int eqLen[3] = {-1, -1, -1};

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

  if (millis() >= nextUpdate) {
    for (int i = 0; i < 3; i++) {
      colorIndex[i] = (colorIndex[i] + 3) % 7;
      pointIndex[i] = 0;
      do {
        equationIndex[i] = (equationIndex[i] + 3) % NUM_EQUATIONS;
        eqLen[i] = setup_equation(equationIndex[i], equations[i], eqSize[i]);
      } while (eqLen[i] == -1);
    }
    nextUpdate = millis() + 30000;
  }

  laser_point_x3_t points;
  memset(&points, 0, sizeof(laser_point_x3_t));

  for (int i = 0; i < 3; i++) {
    points.p[i] = (laser_point_t){
      (uint16_t)(equations[i][pointIndex[i]].x + offsets[i][0]),
      (uint16_t)(equations[i][pointIndex[i]].y + offsets[i][1]),
      equations[i][pointIndex[i]].on ? COLOR_LIST[colorIndex[i]][0] : 0,
      equations[i][pointIndex[i]].on ? COLOR_LIST[colorIndex[i]][1] : 0,
      equations[i][pointIndex[i]].on ? COLOR_LIST[colorIndex[i]][2] : 0
    };

    pointIndex[i] = (pointIndex[i] + 1) % eqLen[i];
    if (pointIndex[i] == 0) {
      offsets[i][0] += dirs[i][0];
      offsets[i][1] += dirs[i][1];
      if      (offsets[i][0] + eqSize[i][0] / 2 > bounds[1] && dirs[i][0] > 0) dirs[i][0] *= -1;
      else if (offsets[i][0] - eqSize[i][0] / 2 < bounds[0] && dirs[i][0] < 0) dirs[i][0] *= -1;
      if      (offsets[i][1] + eqSize[i][1] / 2 > bounds[2] && dirs[i][1] > 0) dirs[i][1] *= -1;
      else if (offsets[i][1] - eqSize[i][1] / 2 < bounds[3] && dirs[i][1] < 0) dirs[i][1] *= -1;
    }
  }

  return points;
}

laser_point_x3_t LaserGenerator::get_spirograph_point() {
  static int setup_complete = 0;
  static uint16_t bounds[4];
  static Spirograph spiro[3] = { Spirograph(), Spirograph(), Spirograph() };
  static double offsets[3][2];
  static double dirs[3][2];
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

      offsets[i][0] = (bounds[0] + bounds[1]) / 2.0;
      offsets[i][1] = (bounds[2] + bounds[3]) / 2.0;
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

  uint16_t x = (uint16_t)(cos(angle * PI / 180.0) * PONG_BALL_RADIUS + ballX);
  uint16_t y = (uint16_t)(sin(angle * PI / 180.0) * PONG_BALL_RADIUS + ballY);
  uint8_t color = angle > 0 ? 255 : 0;
  points.p[ballLaser] = (laser_point_t){x, y, color, color, color};
  angle = (angle + 30) % 420;
  if (angle == 0) *done = true;
  
  return points;
}

laser_point_x3_t LaserGenerator::get_pong_paddles(uint8_t ballLaser, double ballX, double ballY, 
                                  double centerX, double leftPaddle, double rightPaddle, bool *done) {
  static int setup_complete = 0;
  static xy_t paddlePoints[6];
  static int interpLen = 0;
  static xy_t *interpPoints;
  static int index = 0;
  
  if (setup_complete == 0) {
    if (ballLaser == 0) {
      paddlePoints[0] = (xy_t){(int)ballX, (int)ballY, false};
      paddlePoints[1] = (xy_t){(int)(centerX - PONG_PADDLE_GAP), (int)(leftPaddle - PONG_PADDLE_HALF_HEIGHT), false};
      paddlePoints[2] = (xy_t){(int)(centerX - PONG_PADDLE_GAP), (int)(leftPaddle + PONG_PADDLE_HALF_HEIGHT), true};
      paddlePoints[3] = (xy_t){(int)(centerX + PONG_PADDLE_GAP), (int)(rightPaddle + PONG_PADDLE_HALF_HEIGHT), false};
      paddlePoints[4] = (xy_t){(int)(centerX + PONG_PADDLE_GAP), (int)(rightPaddle - PONG_PADDLE_HALF_HEIGHT), true};
      paddlePoints[5] = (xy_t){(int)ballX, (uint16_t)ballY, false};
    } else {
      paddlePoints[0] = (xy_t){(int)(centerX + PONG_PADDLE_GAP), (int)(rightPaddle - PONG_PADDLE_HALF_HEIGHT), false};
      paddlePoints[1] = (xy_t){(int)(centerX - PONG_PADDLE_GAP), (int)(leftPaddle - PONG_PADDLE_HALF_HEIGHT), false};
      paddlePoints[2] = (xy_t){(int)(centerX - PONG_PADDLE_GAP), (int)(leftPaddle + PONG_PADDLE_HALF_HEIGHT), true};
      paddlePoints[3] = (xy_t){(int)(centerX + PONG_PADDLE_GAP), (int)(rightPaddle + PONG_PADDLE_HALF_HEIGHT), false};
      paddlePoints[4] = (xy_t){(int)(centerX + PONG_PADDLE_GAP), (int)(rightPaddle - PONG_PADDLE_HALF_HEIGHT), true};
      paddlePoints[5] = (xy_t){(int)(centerX + PONG_PADDLE_GAP), (int)(rightPaddle - PONG_PADDLE_HALF_HEIGHT), false};
    }

    interpLen = get_interpolated_size(paddlePoints, 6, 8);
    interpPoints = new xy_t[interpLen];
    interpolate_objects(paddlePoints, 6, 8, interpPoints);
    setup_complete = 1;
  }

  laser_point_x3_t points;
  memset(&points, 0, sizeof(laser_point_x3_t));
  uint8_t color = interpPoints[index].on ? 255 : 0;
  points.p[0] = (laser_point_t){
    (uint16_t)interpPoints[index].x,
    (uint16_t)interpPoints[index].y,
    color, color, color
  };

  index = (index + 1) % interpLen;
  if (index == 0) {
    delete[] interpPoints;
    setup_complete = 0;
    *done = true;
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
    bool done = false;
    laser_point_x3_t lp = get_pong_ball(ballLaser, ballX, ballY, &done);
    if (done) drawMode = 1;
    return lp;
  }

  if (drawMode == 1) {
    bool done = false;
    laser_point_x3_t lp = get_pong_paddles(ballLaser, ballX, ballY, centerX, leftPaddle, rightPaddle, &done);
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
        leftPaddle += max(min(lp.y, (int)bounds[3] - PONG_PADDLE_HALF_HEIGHT), (int)bounds[2] + PONG_PADDLE_HALF_HEIGHT) - leftPaddle;
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
        rightPaddle += max(min(lp.y, (int)bounds[3] - PONG_PADDLE_HALF_HEIGHT), (int)bounds[2] + PONG_PADDLE_HALF_HEIGHT) - rightPaddle;
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

  static unsigned long nextUpdate = 0;
  
  static xy_t pointList[WAND_PATH_LENGTH];
  static uint8_t listLen = 0;
  static uint8_t pIndex = 0;
  static uint8_t cIndex = 0;
  static bool forwardDir = true;
  static int currentLaser = -1;

  if (setup_complete == 0) {
    sier.get_laser_rect_interior(bounds);
    setup_complete = 1;
  }

  laser_point_x3_t points;
  memset(&points, 0, sizeof(laser_point_x3_t));
  if (numWandsConnected == 0) return points;

  if (millis() > nextUpdate) {
    nextUpdate = millis() + WAND_DELTA_TIME;
    
    int laserIndex = -1;
    double v[3];
    sier.get_wand_projection(wandData1, &laserIndex, v);
    
    if (laserIndex != currentLaser || laserIndex < 0) {
      listLen = 0;
      pIndex = 0;
      cIndex = 0;
      forwardDir = true;
      currentLaser = laserIndex;
    }

    if (laserIndex < 0) return points;

    if (listLen < WAND_PATH_LENGTH) listLen++;
    pointList[pIndex] = sier.sierpinski_to_laser_coords(laserIndex, v);
    pIndex = (pIndex + 1) % WAND_PATH_LENGTH;
  }

  if (listLen == 0) return points;
    
  if (listLen < WAND_PATH_LENGTH) {
    if (forwardDir) {
      if (cIndex < (listLen - 1)) cIndex++;
      else forwardDir = false;
    } else {
      if (cIndex > 0) cIndex--;
      else forwardDir = true;
    }
  } else {
    if (forwardDir) {
      if (((cIndex + 1) % WAND_PATH_LENGTH) != pIndex) 
        cIndex = (cIndex + 1) % WAND_PATH_LENGTH;
      else 
        forwardDir = false;
    } else {
      if (cIndex != pIndex) 
        cIndex = (cIndex + WAND_PATH_LENGTH - 1) % WAND_PATH_LENGTH;
      else 
        forwardDir = true;
    }
  }

  rgb_t wandColor1 = sier.get_wand_rotation_color(wandData1, 0);
  points.p[currentLaser] = (laser_point_t) {
    (uint16_t)pointList[cIndex].x,
    (uint16_t)pointList[cIndex].y,
    wandColor1.r, wandColor1.g, wandColor1.b
  };

  rgb_t wandColor2 = sier.get_wand_rotation_color(wandData1, 120);
  points.p[(currentLaser + 1) % 3] = (laser_point_t) {
    (uint16_t)pointList[cIndex].x,
    (uint16_t)(bounds[2] + (bounds[3] - pointList[cIndex].y)),
    wandColor2.r, wandColor2.g, wandColor2.b
  };

  rgb_t wandColor3 = sier.get_wand_rotation_color(wandData1, 240);
  points.p[(currentLaser + 2) % 3] = (laser_point_t) {
    (uint16_t)(2048 + (2048 - pointList[cIndex].x)),
    (uint16_t)pointList[cIndex].y,
    wandColor3.r, wandColor3.g, wandColor3.b
  };

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

  uint8_t r = 0;
  uint8_t g = 0;
  uint8_t b = 0;

  if      ((curr_index / 40) % 3 == 0) r = 255;
  else if ((curr_index / 40) % 3 == 1) g = 255;
  else                                 b = 255;

  laser_point_t lp = (laser_point_t){
    (uint16_t)interp_bounds[curr_index].x,
    (uint16_t)interp_bounds[curr_index].y,
    r, g, b
  };
  return (laser_point_x3_t){lp, lp, lp};
}

void LaserGenerator::calibrate_wand(uint16_t x, uint16_t y, uint16_t z, uint16_t w) {
  double q[4] = {
    ((double)x - 16384.0) / 16384.0,
    ((double)y - 16384.0) / 16384.0,
    ((double)z - 16384.0) / 16384.0,
    ((double)w - 16384.0) / 16384.0
  };
  sier.calibrate_wand_position(q);
}
