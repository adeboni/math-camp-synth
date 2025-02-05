#ifndef _LASER_GENERATOR_
#define _LASER_GENERATOR_

#include <Arduino.h>
#include "primitives.h"
#include "sierpinski.h"
#include "laser_objects.h"

#define UDP_AUDIO_BUFF_SIZE 1024

#define PONG_START_SPEED 6
#define PONG_BALL_RADIUS 20
#define PONG_AI_SPEED    5
#define PONG_PADDLE_GAP  60
#define PONG_PADDLE_HALF_HEIGHT 100

#define SOUND_EFFECT_PONG_WALL     0
#define SOUND_EFFECT_PONG_PADDLE   1
#define SOUND_EFFECT_PONG_GAMEOVER 2

#define NUM_CIRCLES 3
#define MAX_CIRCLE_RADIUS 200
#define MAX_CIRCLE_ANGLE 400

#define NUM_EQUATIONS 36

#define WAND_PATH_LENGTH 10
#define WAND_DELTA_TIME 100

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
    double wandData1[4] = {0.0, 0.0, 0.0, 1.0};
    double wandData2[4] = {0.0, 0.0, 0.0, 1.0};
    int playSoundEffect = -1;
    char soundEffects[3][20] = {
      "/pong/wall.wav",
      "/pong/paddle.wav",
      "/pong/gameover.wav"
    };
    

  private:
    Sierpinski sier;
    
    int setup_equation(int index, xy_t *result, int *size);
    laser_point_x3_t get_equation_point();
    laser_point_x3_t get_spirograph_point();
    laser_point_x3_t get_audio_visualizer_point();
    laser_point_x3_t get_pong_point();
    laser_point_x3_t get_pong_ball(uint8_t ballLaser, double ballX, double ballY, bool *done);
    laser_point_x3_t get_pong_paddles(uint8_t ballLaser, double ballX, double ballY, 
                                 double centerX, double leftPaddle, double rightPaddle, bool *done);
    laser_point_x3_t get_wand_drawing_point();
    laser_point_x3_t get_calibration_point();

    uint8_t COLOR_LIST[7][3] = {
      {0, 0, 255}, {0, 255, 0}, {255, 0, 0}, {0, 255, 255}, 
      {255, 255, 0}, {255, 0, 255}, {255, 255, 255}
    };

    uint16_t *EQUATION_LIST[NUM_EQUATIONS] = {
      EQN_01, EQN_02, EQN_03, EQN_04, EQN_05, EQN_06, EQN_07, EQN_08, 
      EQN_09, EQN_10, EQN_11, EQN_12, EQN_13, EQN_14, EQN_15, EQN_16, 
      EQN_17, EQN_18, EQN_19, EQN_20, EQN_21, EQN_22, EQN_23, EQN_24, 
      EQN_25, EQN_26, EQN_27, EQN_28, EQN_29, EQN_30, EQN_31, EQN_32, 
      EQN_33, EQN_34, EQN_35, EQN_36
    };

    int EQUATION_LENS[NUM_EQUATIONS] = {
      210, 224, 324, 322, 156, 106, 412, 318, 
      260, 308, 104, 164, 192, 198, 308, 332, 
      264, 152, 266, 488, 128, 82, 150, 278, 
      214, 500, 564, 572, 318, 156, 86, 128, 
      190, 282, 268, 380
    };
};

#endif
