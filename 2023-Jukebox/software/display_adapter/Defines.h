#pragma once

#define ShiftRegisterPWM_LATCH_PORT PORTB
#define ShiftRegisterPWM_LATCH_MASK 0B00000001
#define ShiftRegisterPWM_DATA_PORT PORTB
#define ShiftRegisterPWM_DATA_MASK 0B00000100
#define ShiftRegisterPWM_CLOCK_PORT PORTB
#define ShiftRegisterPWM_CLOCK_MASK 0B00000010

#define LED_DAT_PIN 10
#define LED_CLK_PIN 9
#define LED_LTC_PIN 8

#define NES_DAT_PIN 13
#define NES_CLK_PIN 7
#define NES_LTC_PIN 6

#define BTN_PIN  A7
#define MODE_PIN A6

#define PWR_BTN 87.5
#define SKP_BTN 85.7
#define LTL_BTN 80.0
#define LTR_BTN 50.0
#define NBL_BTN 83.4
#define NBR_BTN 66.7
#define ENT_BTN 75.0

#define MODE_0 100.0
#define MODE_1 67.0
#define MODE_2 47.2
#define MODE_3 31.6
#define MODE_4 18.6
#define MODE_5 9.0
#define MODE_6 2.7
#define MODE_7 0.0

#define PWR_BTN_LED 7
#define SKP_BTN_LED 6
#define LTL_BTN_LED 4
#define LTR_BTN_LED 1
#define NBL_BTN_LED 5
#define NBR_BTN_LED 2
#define ENT_BTN_LED 3

const int BTN_LED_ARRAY[] = {LTL_BTN_LED, LTR_BTN_LED, NBL_BTN_LED, NBR_BTN_LED, ENT_BTN_LED, SKP_BTN_LED, PWR_BTN_LED};

#define MOTOR_0    13
#define MOTOR_1    14
#define MOTOR_2    15
#define LAMP       12

#define DOT_0      39
#define DOT_1      38
#define DOT_2      37
#define DOT_3      36
#define DOT_4      35
#define DOT_5      34

const int DOT_ARRAY[] = {DOT_0, DOT_1, DOT_2, DOT_3, DOT_4, DOT_5};

#define MOUTH_B_0  21
#define MOUTH_R_0  22
#define MOUTH_W_0  23

#define MOUTH_B_1  18
#define MOUTH_R_1  19
#define MOUTH_W_1  20

#define MOUTH_B_2  31
#define MOUTH_R_2  16
#define MOUTH_W_2  17

#define MOUTH_B_3  28
#define MOUTH_R_3  29
#define MOUTH_W_3  30

#define MOUTH_B_4  25
#define MOUTH_R_4  26
#define MOUTH_W_4  27

const int MOUTH_B_ARRAY[] = {MOUTH_B_0, MOUTH_B_1, MOUTH_B_2, MOUTH_B_3, MOUTH_B_4};
const int MOUTH_R_ARRAY[] = {MOUTH_R_0, MOUTH_R_1, MOUTH_R_2, MOUTH_R_3, MOUTH_R_4};
const int MOUTH_W_ARRAY[] = {MOUTH_W_0, MOUTH_W_1, MOUTH_W_2, MOUTH_W_3, MOUTH_W_4};
const int MOUTH_ARRAY[] = {MOUTH_B_0, MOUTH_B_1, MOUTH_B_2, MOUTH_B_3, MOUTH_B_4, 
                           MOUTH_R_0, MOUTH_R_1, MOUTH_R_2, MOUTH_R_3, MOUTH_R_4,
                           MOUTH_W_0, MOUTH_W_1, MOUTH_W_2, MOUTH_W_3, MOUTH_W_4};

#define ANIMATION_LENGTH_MS 40000
#define POWER_TIMEOUT_MS    3600000
#define POWER_GRACE_MS      10000
