#include "wifi_credentials.h"
#include <SPI.h>
#include <WiFi.h>
#include <WiFiUdp.h>

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
    uint16_t yaw, pitch, rotation;
} wand_data_t;

//TODO: support multiple wands
wand_data_t wandData = {0, 0, 0};

/////////////////////////////////////////////////////////////////////

void setup() {
  pinMode(ESP_BUSY_PIN, OUTPUT);
  digitalWrite(ESP_BUSY_PIN, LOW);
  SPI.begin(ESP_SCK_PIN, ESP_MISO_PIN, ESP_MOSI_PIN, ESP_CS_PIN);
  pinMode(ESP_CS_PIN, INPUT);
  SPI.setDataMode(SPI_MODE0);
  SPI.setBitOrder(MSBFIRST);

  WiFi.softAPConfig(TARGET_IP, TARGET_GATEWAY, TARGET_SUBNET);
  WiFi.softAP(WIFI_NAME, WIFI_PASSWORD);
  udp.begin(TARGET_PORT);
}

void loop() {
  receiveUDP();
  sendData();
}

/////////////////////////////////////////////////////////////////////

void receiveUDP() {
  int packetSize = udp.parsePacket();
  if (packetSize) {
    int len = udp.read(buffer, PACKET_SIZE);
    //TODO
  }
}

void sendData() {
  if (digitalRead(CS_PIN) == LOW) {
    digitalWrite(ESP_BUSY_PIN, HIGH);
    SPI.transfer((uint8_t)(wandData.yaw >> 8));
    SPI.transfer((uint8_t)(wandData.yaw & 0xff));
    SPI.transfer((uint8_t)(wandData.pitch >> 8));
    SPI.transfer((uint8_t)(wandData.pitch & 0xff));
    SPI.transfer((uint8_t)(wandData.rotation >> 8));
    SPI.transfer((uint8_t)(wandData.rotation & 0xff));
  }
}