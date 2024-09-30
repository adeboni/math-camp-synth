#include <I2S.h>
#include <ICM_20948.h>
#include <Adafruit_NeoPixel.h>
#include <WiFi.h>
#include <WiFiUdp.h>

#define F32_TO_INT(X) ((uint16_t)(X * 16384 + 16384))
#define FORCE_BOOT         false
#define DEBUG_PRINT        false
#define TOUCH_PIN          32
#define VBUS_PIN           9
#define CHARGE_PIN         34
#define BATTERY_PIN        35
#define TOUCH_THRESHOLD    40
#define STATE_CHARGING     0
#define STATE_CHARGED      1
#define STATE_UNPLUGGED    2

bool charging = false;
bool wifiConnected = false;
uint8_t vbusPresent = 0;
uint8_t chargeState = 0;
uint16_t batteryLevel = 0;
uint8_t powerState = STATE_UNPLUGGED;

Adafruit_NeoPixel strip(1, 2, NEO_RGB + NEO_KHZ800);
ICM_20948_I2C myICM;
WiFiUDP udp;
const char* udpAddress = "10.0.0.2";
const int udpPort = 5005;

int checkPowerState() {
  static unsigned long lastUpdate = 0;
  if (millis() - lastUpdate < 2000)
    return;
  lastUpdate = millis();

  vbusPresent = digitalRead(VBUS_PIN);
  chargeState = 0;
  for (int i = 0; i < 10; i++)
    chargeState |= digitalRead(CHARGE_PIN);
  batteryLevel = analogRead(BATTERY_PIN);

  if (DEBUG_PRINT) {
    Serial.print("VBUS: ");       Serial.print(vbusPresent);
    Serial.print("   CHARGE: ");  Serial.print(chargeState);
    Serial.print("   BATTERY: "); Serial.println(batteryLevel);
  }

  if (vbus == LOW) 
    powerState = STATE_UNPLUGGED;
  else if (charge == HIGH) 
    powerState = STATE_CHARGED;
  else
    powerState = STATE_CHARGING;
}

void updateLEDCore0(void *parameter) {
  int _r = 0, _g = 0, _b = 0;

  while (1) {
    int r = 0;
    int g = 0;
    int b = 0;

    if (powerState == STATE_CHARGING) {
      r = 255;
    } else if (powerState == STATE_CHARGED) {
      g = 255;
    } else if (wifiConnected) {
      b = 255;
    } else {
      r = 255;
      g = 255;
    }

    if (r != _r || _g != g || _b != b) {
      strip.setPixelColor(0, r, g, b);
      strip.show();
      _r = r; _g = g; _b = b;
    }
    
    delay(500);
  }
}

void initI2S() {
  Serial.print(F("Connecting to microphone... "));
  I2S.setAllPins(14, 15, 25, 25, 25);
  // start I2S at 8 kHz with 16-bits per sample
  if (!I2S.begin(I2S_PHILIPS_MODE, 8000, 16)) {
    Serial.println(F("Failed to initialize I2S!"));
    while (1);
  }
  Serial.println(F("connected"));
}

void initICM() {
  Serial.print(F("Connecting to IMU... "));

  Wire.begin();
  Wire.setClock(400000);
  
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

void initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin("Robbie", "just one two three four five");
  Serial.println(F("Connecting to Wifi... "));

  int timeoutCounter = 0;
  while (WiFi.status() != WL_CONNECTED){
    delay(200);
    timeoutCounter++;
    if (timeoutCounter > 50) {
      Serial.println(F("Failed to connect to Wifi, restarting... "));
      ESP.restart();
    }
  }

  Serial.println(F("Connected to the WiFi network"));
  Serial.print(F("Local IP: "));
  Serial.println(WiFi.localIP());
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
  
  if (digitalRead(VBUS_PIN) == HIGH && !FORCE_BOOT)
    charging = true;
  
  if (!charging) {
    initWifi();
    initI2S();
    initICM();
  }
  initNeopixel();

  TaskHandle_t taskCore0;
  xTaskCreatePinnedToCore(updateLEDCore0, "LEDHandler", 10000, NULL, 0, &taskCore0, 0);
}

uint8_t checkButton() {
  int rawTouch = touchRead(TOUCH_PIN);

  if (DEBUG_PRINT) {
    Serial.print(F("Touch pad: "));
    Serial.println(rawTouch);
  }

  return rawTouch < TOUCH_THRESHOLD ? LOW : HIGH;
}

bool checkICM(uint8_t *buf) {
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

      uint16_t x = F32_TO_INT(q1);
      uint16_t y = F32_TO_INT(q2);
      uint16_t z = F32_TO_INT(q3);
      uint16_t w = F32_TO_INT(q0);
      buf[0] = x & 0xFF;
      buf[1] = (x >> 8) & 0xFF;
      buf[2] = y & 0xFF;
      buf[3] = (y >> 8) & 0xFF;
      buf[4] = z & 0xFF;
      buf[5] = (z >> 8) & 0xFF;
      buf[6] = w & 0xFF;
      buf[7] = (w >> 8) & 0xFF;
      return true;
    }
  }
  return false;
}

bool checkI2S(uint8_t *buf) {
  int sample = I2S.read();
  if (sample && sample != -1 && sample != 1) {
    if (DEBUG_PRINT) {
      Serial.print(F("Audio sample: "));
      Serial.println(sample);
    }
    buf[0] = sample & 0xFF;
    buf[1] = (sample >> 8) & 0xFF;
    return true;
  }
  return false;
}

void sendData() {
  //Targeting 57 packets/second
  uint8_t data[1405]; //1472 max
  data[0] = vbusPresent;
  data[1] = chargeState;
  data[2] = batteryLevel & 0xFF;
  data[3] = (batteryLevel >> 8) & 0xFF;
  data[4] = checkButton();

  uint8_t icmBuf[8];
  uint8_t i2sBuf[2];
  for (int i = 0; i < 140; i++) {
    while (!checkICM(icmBuf));
    while (!checkI2S(i2sBuf));
    memcpy(data + (5 + i * 10), icmBuf, 8);
    memcpy(data + (5 + i * 10 + 8), i2sBuf, 2);
  }

  udp.beginPacket(udpAddress, udpPort);
  udp.write(data);
  udp.endPacket();
}

void loop() {
  checkPowerState();
  if (!charging)
    sendData();
}
