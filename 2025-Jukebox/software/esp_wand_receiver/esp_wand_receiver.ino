#include "wifi_credentials.h"
#include <WiFi.h>
#include <WiFiUdp.h>
#include <map>
#include "SPIS.h"

#define SPI_BUFFER_LEN SPI_MAX_DMA_LEN

#define SAMPLES_PER_PACKET 512
#define PACKET_SIZE        (22 + SAMPLES_PER_PACKET * 2)
WiFiUDP udp;
uint8_t udpBuffer[PACKET_SIZE];

typedef struct {
    uint16_t x, y, z, w;
    uint8_t buttonPressed;
    unsigned long timestamp;
} wand_data_t;

std::map<uint32_t, wand_data_t> packetMap;
uint8_t tempTxbuf[40];
uint8_t* txbuf;
uint8_t* rxbuf;

/////////////////////////////////////////////////////////////////////

TaskHandle_t Core0Task;

void setup() {
  WiFi.softAPConfig(TARGET_IP, TARGET_GATEWAY, TARGET_SUBNET);
  WiFi.softAP(WIFI_NAME, WIFI_PASSWORD);
  udp.begin(TARGET_PORT);

  xTaskCreatePinnedToCore(core0, "Task0", 10000, NULL, 1, &Core0Task, 0);
}

void loop() {
  receiveUDP();
  cleanDictionary();
}

void setup1() {
  Serial.begin(115200);

  SPIS.begin();
  rxbuf = (uint8_t*)heap_caps_malloc(SPI_BUFFER_LEN, MALLOC_CAP_DMA);
  txbuf = (uint8_t*)heap_caps_malloc(SPI_BUFFER_LEN, MALLOC_CAP_DMA);
  memset(rxbuf, 0, SPI_BUFFER_LEN);
  memset(txbuf, 0, SPI_BUFFER_LEN);
  memset(tempTxbuf, 0, 40);
}

void loop1() {
  sendData();
}

void core0(void * pvParameters) {
  setup1();
  while (1) loop1();
}

/////////////////////////////////////////////////////////////////////

void receiveUDP() {
  int packetSize = udp.parsePacket();
  if (packetSize >= 18) {
    uint32_t remoteIP = udp.remoteIP();
    udp.read(udpBuffer, PACKET_SIZE);

    wand_data_t data;
    //data.w = (uint16_t)udpBuffer[11] << 8 | udpBuffer[10];
    //data.x = (uint16_t)udpBuffer[13] << 8 | udpBuffer[12];
    //data.y = (uint16_t)udpBuffer[15] << 8 | udpBuffer[14];
    //data.z = (uint16_t)udpBuffer[17] << 8 | udpBuffer[16];
    //data.buttonPressed = udpBuffer[8];

    data.w = 2;
    data.x = 3;
    data.y = 4;
    data.z = 5;
    data.buttonPressed = 1;
    data.timestamp = millis();

    packetMap.insert_or_assign(remoteIP, data);
    tempTxbuf[0] = packetMap.size();

    uint8_t i = 0;
    for (auto it = packetMap.begin(); it != packetMap.end(), i < 4; it++, i++) {
      tempTxbuf[1 + i * 9] = (uint8_t)(it->second.w >> 8);
      tempTxbuf[2 + i * 9] = (uint8_t)(it->second.w & 0xff);
      tempTxbuf[3 + i * 9] = (uint8_t)(it->second.x >> 8);
      tempTxbuf[4 + i * 9] = (uint8_t)(it->second.x & 0xff);
      tempTxbuf[5 + i * 9] = (uint8_t)(it->second.y >> 8);
      tempTxbuf[6 + i * 9] = (uint8_t)(it->second.y & 0xff);
      tempTxbuf[7 + i * 9] = (uint8_t)(it->second.z >> 8);
      tempTxbuf[8 + i * 9] = (uint8_t)(it->second.z & 0xff);  
      tempTxbuf[9 + i * 9] = it->second.buttonPressed;
    }
  }
}

void cleanDictionary() {
  static unsigned long lastUpdate = 0;
  unsigned long currTime = millis();

  if (currTime - lastUpdate > 1000) {
    for (auto it = packetMap.begin(); it != packetMap.end(); ) {
      if (currTime - it->second.timestamp > 10000) 
        it = packetMap.erase(it);
      else
        ++it;
    }
    
    tempTxbuf[0] = packetMap.size();
    lastUpdate = millis();
  }
}

void sendData() {
  int commandLength = SPIS.transfer(NULL, rxbuf, SPI_BUFFER_LEN);
  Serial.print("1: ");
  Serial.println(commandLength);
  if (commandLength == 4) {
    memcpy(txbuf, tempTxbuf, 40);
    Serial.print("2: ");
    Serial.println(SPIS.transfer(txbuf, NULL, 40));
  }
}
