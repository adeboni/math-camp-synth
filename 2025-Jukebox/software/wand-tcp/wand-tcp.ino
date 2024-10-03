#include "wifi_credentials.h"
#include <I2S.h>
#include <ICM_20948.h>
#include <Adafruit_NeoPixel.h>
#include <WiFi.h>

#define F32_TO_INT(X) ((uint16_t)(X * 16384 + 16384))
#define FORCE_BOOT          false
#define DEBUG_PRINT         false
#define TOUCH_PIN           32
#define VBUS_PIN            9
#define CHARGE_PIN          34
#define BATTERY_PIN         35
#define TOUCH_THRESHOLD     40
#define PWR_STATE_CHARGING  0
#define PWR_STATE_CHARGED   1
#define PWR_STATE_UNPLUGGED 2
#define PWR_STATE_INVALID   3
#define AUDIO_RATE_HZ       16000
#define SAMPLES_PER_PACKET  320 // 16000 / 50 packets per sec
#define PACKET_SIZE         656 // 16 + 320 * 2 (1472 max)

TaskHandle_t taskCore0;
Adafruit_NeoPixel strip(1, 2, NEO_RGB + NEO_KHZ800);
ICM_20948_I2C myICM;
WiFiClient client;
bool wifiConnected = false;
uint8_t powerState = PWR_STATE_INVALID;
uint8_t buffer[PACKET_SIZE];
uint8_t packetBuffer[PACKET_SIZE];
bool dataReadyToSend = false;
int audioSample = 0;


void updateLED() {
  if (wifiConnected) 
    strip.setPixelColor(0, 0, 0, 255);
  else if (powerState == PWR_STATE_CHARGING) 
    strip.setPixelColor(0, 255, 0, 0);
  else if (powerState == PWR_STATE_CHARGED) 
    strip.setPixelColor(0, 0, 255, 0);
  else 
    strip.setPixelColor(0, 255, 255, 0);
  strip.show();
}


///// INIT METHODS /////

void initI2S() {
  Serial.print(F("Connecting to microphone... "));
  I2S.setAllPins(14, 15, 25, 25, 25);
  if (!I2S.begin(I2S_PHILIPS_MODE, AUDIO_RATE_HZ, 16)) {
    Serial.println(F("Failed to initialize I2S!"));
    ESP.restart();
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
    ESP.restart();
  }
  
  Serial.println(F("connected"));
}

void initWiFi() {
  Serial.print(F("Connecting to Wifi... "));
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_NAME, WIFI_PASSWORD);
  
  int timeoutCounter = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(200);
    if (timeoutCounter++ > 150) {
      Serial.println(F("Failed to connect to Wifi, restarting... "));
      ESP.restart();
    }
  }

  WiFi.setSleep(WIFI_PS_NONE);
  Serial.print(F("connected with IP: "));
  Serial.println(WiFi.localIP());

  if (!client.connect(TARGET_IP, TARGET_PORT)) {
    Serial.println(F("TCP connection failed!"));
    ESP.restart();
  }
  
  wifiConnected = true;
  updateLED();
}

void initNeopixel() {
  strip.begin();
  strip.show();
  strip.setBrightness(255);
  updateLED();
}


///// READ DATA /////

void checkPowerState() {
  static uint8_t prevPowerState = PWR_STATE_INVALID;
  static unsigned long lastUpdate = 0;
  if (millis() - lastUpdate < 2000)
    return;
  lastUpdate = millis();

  uint8_t vbusPresent = digitalRead(VBUS_PIN);
  uint8_t chargeState = 0;
  for (int i = 0; i < 10; i++)
    chargeState |= digitalRead(CHARGE_PIN);
  uint16_t batteryLevel = analogRead(BATTERY_PIN);

  if (DEBUG_PRINT) {
    Serial.print("VBUS: ");       Serial.print(vbusPresent);
    Serial.print("   CHARGE: ");  Serial.print(chargeState);
    Serial.print("   BATTERY: "); Serial.println(batteryLevel);
  }

  if (vbusPresent == LOW) 
    powerState = PWR_STATE_UNPLUGGED;
  else if (chargeState == HIGH) 
    powerState = PWR_STATE_CHARGED;
  else
    powerState = PWR_STATE_CHARGING;

  if (powerState != prevPowerState)
    updateLED();
  prevPowerState = powerState;
  
  buffer[0] = vbusPresent;
  buffer[1] = 0;
  buffer[2] = chargeState;
  buffer[3] = 0;
  buffer[4] = batteryLevel & 0xFF;
  buffer[5] = (batteryLevel >> 8) & 0xFF;
}

void checkButton() {
  int rawTouch = touchRead(TOUCH_PIN);

  if (DEBUG_PRINT) {
    Serial.print(F("Touch pad: "));
    Serial.println(rawTouch);
  }

  buffer[6] = rawTouch < TOUCH_THRESHOLD ? 1 : 0;
  buffer[7] = 0;
}

void checkICM() {
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
      buffer[8] = x & 0xFF;
      buffer[9] = (x >> 8) & 0xFF;
      buffer[10] = y & 0xFF;
      buffer[11] = (y >> 8) & 0xFF;
      buffer[12] = z & 0xFF;
      buffer[13] = (z >> 8) & 0xFF;
      buffer[14] = w & 0xFF;
      buffer[15] = (w >> 8) & 0xFF;
    }
  }
}

bool checkI2S() {
  audioSample = I2S.read();
  if (audioSample && audioSample != -1 && audioSample != 1) {
    if (DEBUG_PRINT) {
      Serial.print(F("Audio sample: "));
      Serial.println(audioSample);
    }
    return true;
  }
  return false;
}


///// CORE FUNCTIONS /////

void runCore0(void *parameter) {
  while (1) {
    for (int i = 0; i < SAMPLES_PER_PACKET; i++) {
      while (!checkI2S());
      buffer[16 + i * 2] = audioSample & 0xFF;
      buffer[16 + i * 2 + 1] = (audioSample >> 8) & 0xFF;
    }
    memcpy(packetBuffer, buffer, PACKET_SIZE);
    dataReadyToSend = true;
  }
}

void setup() { 
  Serial.begin(115200);
  Serial.println(F("Booting... "));
  pinMode(VBUS_PIN, INPUT);
  pinMode(CHARGE_PIN, INPUT);
  pinMode(BATTERY_PIN, INPUT);
  analogReadResolution(12);
  initNeopixel();
  
  bool charging = (digitalRead(VBUS_PIN) == HIGH && !FORCE_BOOT);
  if (!charging) {
    initWiFi();
    initI2S();
    initICM();
  }

  xTaskCreatePinnedToCore(runCore0, "TaskCore0", 10000, NULL, 0, &taskCore0, 0);
}

void loop() {
  checkPowerState();

  if (wifiConnected) {
    checkButton();
    checkICM();

    if (dataReadyToSend) {
      client.write(packetBuffer, PACKET_SIZE);
      dataReadyToSend = false;
    }
  }
}
