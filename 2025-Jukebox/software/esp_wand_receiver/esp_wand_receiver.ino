#include "wifi_credentials.h"
#include <SPI.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <map>

#define ESP_SCK_PIN   18
#define ESP_MOSI_PIN  14
#define ESP_MISO_PIN  23
#define ESP_CS_PIN    5
#define ESP_BUSY_PIN  33

#define SAMPLES_PER_PACKET 512
#define PACKET_SIZE        (22 + SAMPLES_PER_PACKET * 2)
WiFiUDP udp;
uint8_t buffer[PACKET_SIZE];

typedef struct {
    uint16_t x, y, z, w;
    uint8_t buttonPressed;
    unsigned long timestamp;
} wand_data_t;

std::map<uint32_t, wand_data_t> packetMap;

/////////////////////////////////////////////////////////////////////

TaskHandle_t Core0Task;

void setup() {
  WiFi.softAPConfig(TARGET_IP, TARGET_GATEWAY, TARGET_SUBNET);
  WiFi.softAP(WIFI_NAME, WIFI_PASSWORD);
  udp.begin(TARGET_PORT);

  xTaskCreatePinnedToCore(core0, "Core 0 task", 10000, NULL, 1, &Core0Task, 0);
}

void loop() {
  receiveUDP();
  cleanDictionary();
}

void core0() {
  pinMode(ESP_BUSY_PIN, OUTPUT);
  digitalWrite(ESP_BUSY_PIN, LOW);
  SPI.begin(ESP_SCK_PIN, ESP_MISO_PIN, ESP_MOSI_PIN, ESP_CS_PIN);
  pinMode(ESP_CS_PIN, INPUT);
  SPI.setDataMode(SPI_MODE0);
  SPI.setBitOrder(MSBFIRST);

  while (1) 
    sendData();
}

/////////////////////////////////////////////////////////////////////

void receiveUDP() {
  int packetSize = udp.parsePacket();
  if (packetSize >= 18) {
    uint32_t remoteIP = udp.remoteIP();
    udp.read(buffer, PACKET_SIZE);

    wand_data_t data;
    data.w = (uint16_t)buffer[11] << 8 | buffer[10];
    data.x = (uint16_t)buffer[13] << 8 | buffer[12];
    data.y = (uint16_t)buffer[15] << 8 | buffer[14];
    data.z = (uint16_t)buffer[17] << 8 | buffer[16];
    data.buttonPressed = buffer[8];
    data.timestamp = millis();

    auto result = packetMap.insert({remoteIP, data});
    if (!result.second) result.first->second = data;

    digitalWrite(ESP_BUSY_PIN, HIGH);
  }
}

void cleanDictionary() {
  static unsigned long lastUpdate = 0;
  unsigned long currTime = millis();

  if (currTime - lastUpdate > 1000) {
    for (auto it = packetMap.begin(); it != packetMap.end(); it++) {
      if (currTime - it->second.timestamp > 10000)
        packetMap.erase(it->first);
    }

    lastUpdate = millis();
  }
}

void sendData() {
  if (digitalRead(CS_PIN) == LOW) {
    uint8_t command = SPI.transfer(0);
    if (command == 0) {
      int numKeys = packetMap.size();
      if (numKeys > 255) numKeys = 255;
      SPI.transfer((uint8_t)numKeys);
    } else if (command <= 4 && command <= packetMap.size()) {
      digitalWrite(ESP_BUSY_PIN, LOW);
      int index = 0;
      for (auto it = packetMap.begin(); it != packetMap.end(); it++) {
        if (index == command - 1) {
          SPI.transfer((uint8_t)(it->second.w >> 8));
          SPI.transfer((uint8_t)(it->second.w & 0xff));
          SPI.transfer((uint8_t)(it->second.x >> 8));
          SPI.transfer((uint8_t)(it->second.x & 0xff));
          SPI.transfer((uint8_t)(it->second.y >> 8));
          SPI.transfer((uint8_t)(it->second.y & 0xff));
          SPI.transfer((uint8_t)(it->second.z >> 8));
          SPI.transfer((uint8_t)(it->second.z & 0xff));
          SPI.transfer(it->second.buttonPressed);
          break;
        }
        index++;
      }
    }
  }
}