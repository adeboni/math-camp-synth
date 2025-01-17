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
  
  rxbuf = (uint8_t*)heap_caps_malloc(SPI_MAX_DMA_LEN, MALLOC_CAP_DMA);
  txbuf = (uint8_t*)heap_caps_malloc(SPI_MAX_DMA_LEN, MALLOC_CAP_DMA);
  memset(rxbuf, 0, SPI_MAX_DMA_LEN);
  memset(txbuf, 0, SPI_MAX_DMA_LEN);
  memset(tempTxbuf, 0, 40);
  
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
    int numKeys = packetMap.size();
    if (numKeys > 255) numKeys = 255;
    tempTxbuf[0] = (uint8_t)numKeys;

    uint8_t index = 0;
    int i = 1;
    for (auto it = packetMap.begin(); it != packetMap.end(), index < 4; it++, index++) {
      tempTxbuf[i++] = (uint8_t)(it->second.w >> 8);
      tempTxbuf[i++] = (uint8_t)(it->second.w & 0xff);
      tempTxbuf[i++] = (uint8_t)(it->second.x >> 8);
      tempTxbuf[i++] = (uint8_t)(it->second.x & 0xff);
      tempTxbuf[i++] = (uint8_t)(it->second.y >> 8);
      tempTxbuf[i++] = (uint8_t)(it->second.y & 0xff);
      tempTxbuf[i++] = (uint8_t)(it->second.z >> 8);
      tempTxbuf[i++] = (uint8_t)(it->second.z & 0xff);  
      tempTxbuf[i++] = it->second.buttonPressed;
    }
  }
}

void cleanDictionary() {
  static unsigned long lastUpdate = 0;
  unsigned long currTime = millis();

  if (currTime - lastUpdate > 1000) {
    for (auto it = packetMap.begin(); it != packetMap.end(); it++)
      if (currTime - it->second.timestamp > 10000)
        packetMap.erase(it->first);
    
    int numKeys = packetMap.size();
    if (numKeys > 255) numKeys = 255;
    tempTxbuf[0] = (uint8_t)numKeys;
   
    lastUpdate = millis();
  }
}

int transfer(uint8_t out[], uint8_t in[], size_t len) {
  spi_slave_transaction_t slvTrans;
  spi_slave_transaction_t* slvRetTrans;

  memset(&slvTrans, 0x00, sizeof(slvTrans));

  slvTrans.length = len * 8;
  slvTrans.trans_len = 0;
  slvTrans.tx_buffer = out;
  slvTrans.rx_buffer = in;

  spi_slave_queue_trans(VSPI_HOST, &slvTrans, portMAX_DELAY);
  digitalWrite(ESP_BUSY_PIN, HIGH);
  spi_slave_get_trans_result(VSPI_HOST, &slvRetTrans, portMAX_DELAY);
  digitalWrite(ESP_BUSY_PIN, LOW);

  return (slvTrans.trans_len / 8);
}

void sendData() {
  int commandLength = transfer(NULL, rxbuf, SPI_MAX_DMA_LEN);
  Serial.print("1: ");
  Serial.println(commandLength);
  if (commandLength == 0) return;
  memcpy(txbuf, tempTxbuf, 40);
  Serial.print("2: ");
  Serial.println(transfer(txbuf, NULL, 40));
}
