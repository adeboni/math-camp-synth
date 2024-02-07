#include <I2S.h>
#include <ICM_20948.h>
#include <BleGamepad.h>
#include <Adafruit_NeoPixel.h>

#define F32_TO_INT(X) ((uint16_t)(X * 16384 + 16384))
#define DEBUG_PRINT        0
#define TOUCH_PIN          32
#define VBUS_PIN           9
#define CHARGE_PIN         34
#define BATTERY_PIN        35
#define TOUCH_THRESHOLD    40
#define STATE_CHARGING     0
#define STATE_CHARGED      1
#define STATE_UNPLUGGED    2
#define MODE_MOTION        0
#define MODE_MICROPHONE    1

int currentMode = MODE_MOTION;
Adafruit_NeoPixel strip(1, 2, NEO_RGB + NEO_KHZ800);
BleGamepad bleGamepad("Math Camp Wand 1", "Alex DeBoni", 100);
ICM_20948_I2C myICM;

int getState() {
  int vbus = digitalRead(VBUS_PIN);
  int charge = 0;
  for (int i = 0; i < 10; i++)
    charge |= digitalRead(CHARGE_PIN);
  int battery = analogRead(BATTERY_PIN);

  if (DEBUG_PRINT) {
    Serial.print("VBUS: ");       Serial.print(vbus);
    Serial.print("   CHARGE: ");  Serial.print(charge);
    Serial.print("   BATTERY: "); Serial.println(battery);
  }

  if (vbus == LOW) return STATE_UNPLUGGED;
  else if (charge == HIGH) return STATE_CHARGED;
  return STATE_CHARGING;
}

void updateLEDCore0(void *parameter) {
  int _r = 0, _g = 0, _b = 0;

  while (1) {
    int r = currentMode == MODE_MICROPHONE ? 255 : 0;
    int g = bleGamepad.isConnected() ? 255 : 0;
    int b = getState() == STATE_CHARGED ? 255 : 0;
    
    if (r == 0 && g == 0 && b == 0)
      r = g = b = 255;

    if (r != _r || _g != g || _b != b) {
      strip.setPixelColor(0, r, g, b);
      strip.show();
      _r = r; _g = g; _b = b;
    }
    
    delay(500);
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
  
  int i2cAddr = 0;
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
  strip.setBrightness(255);
}

void setup() {
  Serial.begin(115200);
  pinMode(VBUS_PIN, INPUT);
  pinMode(CHARGE_PIN, INPUT);
  pinMode(BATTERY_PIN, INPUT);
  analogReadResolution(12);
  
  initI2S();
  initICM();
  initGamepad();
  initNeopixel();

  TaskHandle_t taskCore0;
  xTaskCreatePinnedToCore(updateLEDCore0, "LEDHandler", 10000, NULL, 0, &taskCore0, 0);
}

bool checkBattery() {
  static unsigned long lastUpdate = 0;
  if (millis() - lastUpdate < 5000)
    return false;
    
  lastUpdate = millis();
  int rawAdc = analogRead(BATTERY_PIN);
  double volts = ((double)rawAdc) / 256.0;
  int percent = (int)(359.0 - 265.0 * volts + 48.1 * volts * volts);
  
  if (DEBUG_PRINT) {
    Serial.print(F("Battery voltage: "));
    Serial.println(volts);
    Serial.print(F("Calculated percent: "));
    Serial.println(percent);
  }
  
  bleGamepad.setBatteryLevel(constrain(percent, 0, 100));
  return true;
}

bool checkButton() {
  static int prevTouchState = HIGH;
  int rawTouch = touchRead(TOUCH_PIN);

  if (DEBUG_PRINT) {
    Serial.print(F("Touch pad: "));
    Serial.println(rawTouch);
  }

  int touched = rawTouch < TOUCH_THRESHOLD ? LOW : HIGH;
  if (touched != prevTouchState) {
    if (touched == LOW) bleGamepad.press(BUTTON_1);
    else bleGamepad.release(BUTTON_1);
    prevTouchState = touched;
    return true;
  }
  
  return false;
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

      if (DEBUG_PRINT) {
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
      
      bleGamepad.setAxes(F32_TO_INT(q1), F32_TO_INT(q2), F32_TO_INT(q3), F32_TO_INT(q0), 0, 0, 0, 0);
      return true;
    }
  }
  
  return false;
}

void checkI2S() {
  int sample = I2S.read();
  if (sample && sample != -1 && sample != 1) {
    if (DEBUG_PRINT) {
      Serial.print(F("Audio sample: "));
      Serial.println(sample);
    }
  }
}

void loop() {
  if (currentMode == MODE_MOTION) {
    bool dataChanged = false;
    dataChanged |= checkButton();
    dataChanged |= checkICM();
    dataChanged |= checkBattery();
    if (dataChanged)
      bleGamepad.sendReport();
  } else if (currentMode == MODE_MICROPHONE) {
    checkI2S();
  }
}
