#include "ICM_20948.h"
#include <BleGamepad.h>

#define TOUCH_PIN 32
int prevTouchState = HIGH;
int touchThreshold = 40;

BleGamepad bleGamepad("Math Camp Wand", "Alex DeBoni", 100);
ICM_20948_I2C myICM;

void setup() {
  Serial.begin(115200);
  Wire.begin();
  Wire.setClock(400000);
    
  bool initialized = false;
  while (!initialized) {
    myICM.begin(Wire, 1);
    if (myICM.status != ICM_20948_Stat_Ok)
      delay(500);
    else
      initialized = true;
  }

  bool success = true;
  success &= (myICM.initializeDMP() == ICM_20948_Stat_Ok);
  success &= (myICM.enableDMPSensor(INV_ICM20948_SENSOR_ORIENTATION) == ICM_20948_Stat_Ok);
  success &= (myICM.setDMPODRrate(DMP_ODR_Reg_Quat9, 0) == ICM_20948_Stat_Ok);
  success &= (myICM.enableFIFO() == ICM_20948_Stat_Ok);
  success &= (myICM.enableDMP() == ICM_20948_Stat_Ok);
  success &= (myICM.resetDMP() == ICM_20948_Stat_Ok);
  success &= (myICM.resetFIFO() == ICM_20948_Stat_Ok);

  if (!success) {
    Serial.println(F("Enable DMP failed!"));
    while (1) ;
  }

  BleGamepadConfiguration bleGamepadConfig;
  bleGamepadConfig.setAutoReport(false);
  bleGamepadConfig.setButtonCount(128);
  bleGamepadConfig.setHatSwitchCount(2);
  bleGamepad.begin(&bleGamepadConfig);
}

uint16_t doubleToInt(double x) {
  return (uint16_t)(x * 16384 + 16384);
}

void printValues(double q1, double q2, double q3, double q0) {
  Serial.print(F("{\"quat_w\":"));
  Serial.print(q0, 3);
  Serial.print(F(", \"quat_x\":"));
  Serial.print(q1, 3);
  Serial.print(F(", \"quat_y\":"));
  Serial.print(q2, 3);
  Serial.print(F(", \"quat_z\":"));
  Serial.print(q3, 3);
  Serial.println(F("}"));
}

void loop() {
  //if (!bleGamepad.isConnected())
  //  return;

  bool dataChanged = false;
  int _rawTouch = touchRead(TOUCH_PIN);
  //Serial.println(_rawTouch);
  int touched = _rawTouch < touchThreshold ? LOW : HIGH;
  if (touched != prevTouchState) {
    if (touched == LOW) bleGamepad.press(BUTTON_1);
    else bleGamepad.release(BUTTON_1);
    prevTouchState = touched;
    dataChanged = true;
  }
  
  icm_20948_DMP_data_t data;
  myICM.readDMPdataFromFIFO(&data);

  if ((myICM.status == ICM_20948_Stat_Ok) || (myICM.status == ICM_20948_Stat_FIFOMoreDataAvail)) {
    if ((data.header & DMP_header_bitmap_Quat9) > 0) {
      double q1 = ((double)data.Quat9.Data.Q1) / 1073741824.0;
      double q2 = ((double)data.Quat9.Data.Q2) / 1073741824.0;
      double q3 = ((double)data.Quat9.Data.Q3) / 1073741824.0;
      double q0 = sqrt(1.0 - ((q1 * q1) + (q2 * q2) + (q3 * q3)));
      //printValues(q1, q2, q3, q0);
      bleGamepad.setAxes(doubleToInt(q1), doubleToInt(q2), doubleToInt(q3), doubleToInt(q0), 0, 0, 0, 0);
      dataChanged = true;
    }
  }

  if (dataChanged)
    bleGamepad.sendReport();
}
