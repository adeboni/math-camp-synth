#include "wifi_credentials.h"
#include <WiFi.h>
#include <WiFiUdp.h>
#include <map>
#include <driver/gpio.h>
#include <driver/spi_common.h>
#include <driver/spi_slave.h>

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

DMA_ATTR uint8_t sendbuf[40];

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

void core0(void * pvParameters) {
  memset(sendbuf, 0, 40);
  
  pinMode(ESP_BUSY_PIN, OUTPUT);
  digitalWrite(ESP_BUSY_PIN, LOW);

  spi_bus_config_t busCfg;
  spi_slave_interface_config_t slvCfg;

  memset(&busCfg, 0x00, sizeof(busCfg));
  busCfg.mosi_io_num = ESP_MOSI_PIN;
  busCfg.miso_io_num = ESP_MISO_PIN;
  busCfg.sclk_io_num = ESP_SCK_PIN;

  memset(&slvCfg, 0x00, sizeof(slvCfg));
  slvCfg.mode = 0;
  slvCfg.spics_io_num = ESP_CS_PIN;
  slvCfg.queue_size = 1;
  slvCfg.flags = 0;
  slvCfg.post_setup_cb = NULL;
  slvCfg.post_trans_cb = NULL;

  gpio_set_pull_mode((gpio_num_t)ESP_MOSI_PIN, GPIO_FLOATING);
  gpio_set_pull_mode((gpio_num_t)ESP_SCK_PIN,  GPIO_PULLDOWN_ONLY);
  gpio_set_pull_mode((gpio_num_t)ESP_CS_PIN,   GPIO_PULLUP_ONLY);

  spi_slave_initialize(VSPI_HOST, &busCfg, &slvCfg, 1);

  while (1) {
    sendData();
    yield();
  }
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

    //Prep data for SPI
    int numKeys = packetMap.size();
    if (numKeys > 255) numKeys = 255;
    sendbuf[0] = (uint8_t)numKeys;

    uint8_t index = 0;
    int i = 1;
    for (auto it = packetMap.begin(); it != packetMap.end(), index < 4; it++, index++) {
      sendbuf[i++] = (uint8_t)(it->second.w >> 8);
      sendbuf[i++] = (uint8_t)(it->second.w & 0xff);
      sendbuf[i++] = (uint8_t)(it->second.x >> 8);
      sendbuf[i++] = (uint8_t)(it->second.x & 0xff);
      sendbuf[i++] = (uint8_t)(it->second.y >> 8);
      sendbuf[i++] = (uint8_t)(it->second.y & 0xff);
      sendbuf[i++] = (uint8_t)(it->second.z >> 8);
      sendbuf[i++] = (uint8_t)(it->second.z & 0xff);  
    }

    digitalWrite(ESP_BUSY_PIN, HIGH);
  }
}

void cleanDictionary() {
  static unsigned long lastUpdate = 0;
  unsigned long currTime = millis();

  if (currTime - lastUpdate > 1000) {
    for (auto it = packetMap.begin(); it != packetMap.end(); it++)
      if (currTime - it->second.timestamp > 10000)
        packetMap.erase(it->first);
    lastUpdate = millis();
  }
}

void sendData() {
  if (digitalRead(ESP_CS_PIN) == LOW) {
    digitalWrite(ESP_BUSY_PIN, LOW);
    
    spi_slave_transaction_t slvTrans;
    spi_slave_transaction_t* slvRetTrans;

    memset(&slvTrans, 0x00, sizeof(slvTrans));

    slvTrans.length = 40 * 8;
    slvTrans.trans_len = 0;
    slvTrans.tx_buffer = sendbuf;
    slvTrans.rx_buffer = NULL;

    spi_slave_queue_trans(VSPI_HOST, &slvTrans, portMAX_DELAY);
    spi_slave_get_trans_result(VSPI_HOST, &slvRetTrans, portMAX_DELAY);
  }
}
