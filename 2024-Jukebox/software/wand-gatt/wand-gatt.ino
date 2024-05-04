#include <I2S.h>
#include <ICM_20948.h>
#include <Adafruit_NeoPixel.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

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
#define MODE_MOTION        0
#define MODE_MICROPHONE    1
#define MODE_CHARGING      2

int currentMode = MODE_MOTION;
Adafruit_NeoPixel strip(1, 2, NEO_RGB + NEO_KHZ800);
ICM_20948_I2C myICM;

bool bleConnected = false;
#define SERVER_NAME "Math Camp Wand 1"
#define SERVICE_UUID "64a70011-f691-4b93-a6f4-0968f5b648f8"
BLECharacteristic buttonCharacteristics("64a7000d-f691-4b93-a6f4-0968f5b648f8", BLECharacteristic::PROPERTY_NOTIFY);
BLEDescriptor buttonDescriptor(BLEUUID((uint16_t)0x2902));
BLECharacteristic quaternionCharacteristics("64a70002-f691-4b93-a6f4-0968f5b648f8", BLECharacteristic::PROPERTY_NOTIFY);
BLEDescriptor quaternionDescriptor(BLEUUID((uint16_t)0x2902));

class ConnectionCallbacks: public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) { bleConnected = true; delay(1000); }
  void onDisconnect(BLEServer* pServer) { bleConnected = false; }
};

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
    int r = 0;
    int g = 0;
    int b = 0;
     
    switch (currentMode) {
      case (MODE_CHARGING):
        if (getState() == STATE_CHARGING) r = 255;
        else g = 255;
        break;
      case (MODE_MICROPHONE):
        g = 255;
        break;
      case (MODE_MOTION):
        if (bleConnected) b = 255;
        else r = 255;
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
  if (currentMode != MODE_MICROPHONE) return;
  I2S.setAllPins(14, 15, 25, 25, 25);
  // start I2S at 8 kHz with 32-bits per sample
  if (!I2S.begin(I2S_PHILIPS_MODE, 8000, 32)) {
    Serial.println(F("Failed to initialize I2S!"));
    while (1);
  }
}

void initICM() {
  if (currentMode != MODE_MOTION) return;
  
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

void initBLE() {
  if (currentMode != MODE_MOTION) return;

  Serial.print(F("Setting up Bluetooth... "));
  BLEDevice::init(SERVER_NAME);
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new ConnectionCallbacks());
  BLEService *bmeService = pServer->createService(SERVICE_UUID);

  bmeService->addCharacteristic(&buttonCharacteristics);
  buttonDescriptor.setValue("Button");
  buttonCharacteristics.addDescriptor(&buttonDescriptor);

  bmeService->addCharacteristic(&quaternionCharacteristics);
  quaternionDescriptor.setValue("Quaternion");
  quaternionCharacteristics.addDescriptor(&quaternionDescriptor);
  
  bmeService->start();
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pServer->getAdvertising()->start();
  Serial.print(F("Done"));
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
    currentMode = MODE_CHARGING;
  
  initI2S();
  initICM();
  initBLE();
  initNeopixel();

  TaskHandle_t taskCore0;
  xTaskCreatePinnedToCore(updateLEDCore0, "LEDHandler", 10000, NULL, 0, &taskCore0, 0);
}

void checkBattery() {
  static unsigned long lastUpdate = 0;
  if (millis() - lastUpdate < 5000)
    return;
    
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
}

void checkButton() {
  if (!bleConnected) return;
  static int prevTouchState = HIGH;
  int rawTouch = touchRead(TOUCH_PIN);

  if (DEBUG_PRINT) {
    Serial.print(F("Touch pad: "));
    Serial.println(rawTouch);
  }

  uint8_t touched = rawTouch < TOUCH_THRESHOLD ? LOW : HIGH;
  if (touched != prevTouchState) {
    uint8_t data[1] = { touched };
    buttonCharacteristics.setValue(data, 1);
    buttonCharacteristics.notify();
    prevTouchState = touched;
  }
}

void checkICM() {
  if (!bleConnected) return;
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
      uint8_t data[8] = { (uint8_t)x, (uint8_t)(x >> 8), 
                          (uint8_t)y, (uint8_t)(y >> 8),
                          (uint8_t)z, (uint8_t)(z >> 8),
                          (uint8_t)w, (uint8_t)(w >> 8) };
      quaternionCharacteristics.setValue(data, 8);
      quaternionCharacteristics.notify();
    }
  }
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
    checkButton();
    checkICM();
  } else if (currentMode == MODE_MICROPHONE) {
    checkI2S();
  }
  checkBattery();
}
