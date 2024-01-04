#include <I2S.h>
#include <ICM_20948.h>
#include <BleGamepad.h>
#include <Adafruit_NeoPixel.h>

#define F32_TO_INT(X) ((uint16_t)(X * 16384 + 16384))
#define TOUCH_THRESHOLD 40

Adafruit_NeoPixel strip(1, 2, NEO_GRB + NEO_KHZ800);
BleGamepad bleGamepad("Math Camp Wand", "Alex DeBoni", 100);
ICM_20948_I2C myICM;

void rainbow(int wait) {
  for (long pixelHue = 0; pixelHue < 5 * 65536; pixelHue += 256) {
    strip.setPixelColor(0, strip.gamma32(strip.ColorHSV(pixelHue)));
    strip.show();
    delay(wait);
  }
} 

void initI2S() {
  I2S.setAllPins(14, 15, 25, 25, 25);
  // start I2S at 8 kHz with 32-bits per sample
  if (!I2S.begin(I2S_PHILIPS_MODE, 8000, 32)) {
    Serial.println(F("Failed to initialize I2S!"));
    while (1);
  }
}

void initICM() {
  Wire.begin();
  Wire.setClock(400000);
  
  Serial.print(F("Connecting to ICM 20948... "));
  
  int i2cAddr = 1;
  while (1) {
    myICM.begin(Wire, i2cAddr);
    if (myICM.status == ICM_20948_Stat_Ok)
      break;
	i2cAddr = 1 - i2cAddr;
    delay(500);
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
    while (1);
  }
  
  Serial.println(F("Connected"));
}

void initGamepad() {
  BleGamepadConfiguration bleGamepadConfig;
  bleGamepadConfig.setAutoReport(false);
  bleGamepadConfig.setButtonCount(128);
  bleGamepadConfig.setHatSwitchCount(2);
  bleGamepad.begin(&bleGamepadConfig);
}

void initNeopixel() {
  strip.begin();
  strip.show();
  strip.setBrightness(100);
}

void setup() {
  Serial.begin(115200);
  initI2S();
  initICM();
  initGamepad();
  initNeopixel();

  rainbow(10);
}

bool checkButton() {
  static int prevTouchState = HIGH;
  int rawTouch = touchRead(32);
  //Serial.println(rawTouch);
  int touched = rawTouch < TOUCH_THRESHOLD ? LOW : HIGH;
  if (touched != prevTouchState) {
    if (touched == LOW) bleGamepad.press(BUTTON_1);
    else bleGamepad.release(BUTTON_1);
    prevTouchState = touched;
    return true;
  }
  
  return false;
}

void printQuaternion(double q1, double q2, double q3, double q0) {
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

bool checkICM() {
  icm_20948_DMP_data_t data;
  myICM.readDMPdataFromFIFO(&data);
  if ((myICM.status == ICM_20948_Stat_Ok) || (myICM.status == ICM_20948_Stat_FIFOMoreDataAvail)) {
    if ((data.header & DMP_header_bitmap_Quat9) > 0) {
      double q1 = ((double)data.Quat9.Data.Q1) / 1073741824.0;
      double q2 = ((double)data.Quat9.Data.Q2) / 1073741824.0;
      double q3 = ((double)data.Quat9.Data.Q3) / 1073741824.0;
      double q0 = sqrt(1.0 - ((q1 * q1) + (q2 * q2) + (q3 * q3)));
      //printQuaternion(q1, q2, q3, q0);
      bleGamepad.setAxes(F32_TO_INT(q1), F32_TO_INT(q2), F32_TO_INT(q3), F32_TO_INT(q0), 0, 0, 0, 0);
      return true;
    }
  }
  
  return false;
}

void checkI2S() {
  int sample = I2S.read();
  if (sample && sample != -1 && sample != 1) {
    Serial.println(sample);
  }
}

void loop() {
  bool dataChanged = false;
  dataChanged |= checkButton();
  dataChanged |= checkICM();
  //checkI2S();
  if (dataChanged)
    bleGamepad.sendReport();
}
