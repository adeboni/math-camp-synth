#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUdp.h>

#define BTN_2_PIN     8
#define BTN_1_PIN     9
#define ESP_SCK_PIN   10
#define ESP_MOSI_PIN  11
#define ESP_MISO_PIN  12
#define ESP_CS_PIN    13
#define ESP_RST_PIN   14
#define ESP_RDY_PIN   15
#define WIZ_MISO_PIN  16
#define WIZ_CS_PIN    17
#define WIZ_SCK_PIN   18
#define WIZ_MOSI_PIN  19
#define WIZ_RST_PIN   20
#define WIZ_INT_PIN   21
#define SEG_DIG1_PIN  22
#define SEG_SER_PIN   23
#define SEG_SCK_PIN   24
#define SEG_EN_PIN    25

#define JUKEBOX_MODE_MUSIC   0
#define JUKEBOX_MODE_SYNTH   1
#define JUKEBOX_MODE_EFFECTS 2
#define JUKEBOX_MODE_INVALID 255

#define PACKET_ID_ROBBIE_MODE    1
#define PACKET_ID_LASER_DATA     2
#define PACKET_ID_AUDIO_DATA     3
#define PACKET_ID_BUTTON_PRESS   4
#define PACKET_ID_AUDIO_METADATA 5
#define PACKET_ID_PLAY_EFFECT    6
#define PACKET_ID_WAND_DATA      7
#define PACKET_ID_JUKEBOX_MODE   8

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0x00, 0x33 };
IPAddress ip(10, 0, 0, 33);
EthernetUDP udp;
#define PACKET_BUF_SIZE 1472
uint8_t packetBuffer[PACKET_BUF_SIZE];
IPAddress ioIP(10, 0, 0, 31);
IPAddress jukeboxIP(10, 0, 0, 32);

typedef struct {
    uint16_t w, x, y, z;
    uint8_t buttonPressed;
} wand_data_t;

SPISettings spiSettings(8000000, MSBFIRST, SPI_MODE0);
uint8_t spiBuffer[40];
uint8_t numWandsConnected = 0;
wand_data_t wandData[4];

uint8_t sendMode = 0;
uint8_t currentRobbieMode = 0;
uint8_t MODE_MAPPING[10] = {
  JUKEBOX_MODE_INVALID, 
  JUKEBOX_MODE_MUSIC,
  JUKEBOX_MODE_MUSIC,
  JUKEBOX_MODE_MUSIC,
  JUKEBOX_MODE_MUSIC,
  JUKEBOX_MODE_EFFECTS,
  JUKEBOX_MODE_EFFECTS,
  JUKEBOX_MODE_EFFECTS,
  JUKEBOX_MODE_SYNTH,
  JUKEBOX_MODE_EFFECTS
};

const uint8_t seg_lookup[10] = {
  //bafgedcp
  0b11101110, //0
  0b10000010, //1
  0b11011100, //2
  0b11010110, //3
  0b10110010, //4
  0b01110110, //5
  0b01111110, //6
  0b11000010, //7
  0b11111110, //8
  0b11110110, //9
};

/////////////////////////////////////////////////////////////////////

void setup1() {
  pinMode(WIZ_RST_PIN, OUTPUT);
  digitalWrite(WIZ_RST_PIN, LOW);
  delay(50);
  digitalWrite(WIZ_RST_PIN, HIGH);
  delay(750);
  
  Ethernet.init(WIZ_CS_PIN);
  Ethernet.begin(mac, ip);
  Ethernet.setRetransmissionCount(0);
  Ethernet.setRetransmissionTimeout(0);
  udp.begin(8888);
}

void loop1() {
  checkForPacket();
  sendLaserData();
  sendButtonData();
  sendWandData();
}

void setup() { 
  memset(spiBuffer, 0, 40);

  for (int i = 0; i < 4; i++)
    wandData[i] = (wand_data_t){16384, 16384, 16384, 16384, 0};

  pinMode(BTN_1_PIN, INPUT_PULLUP);
  pinMode(BTN_2_PIN, INPUT_PULLUP);
  pinMode(SEG_DIG1_PIN, OUTPUT);
  pinMode(SEG_SER_PIN, OUTPUT);
  pinMode(SEG_SCK_PIN, OUTPUT);
  pinMode(SEG_EN_PIN, OUTPUT);
  digitalWrite(SEG_DIG1_PIN, LOW);
  updateSegDisplay();

  pinMode(ESP_CS_PIN, OUTPUT);
  digitalWrite(ESP_CS_PIN, HIGH);
  pinMode(ESP_RDY_PIN, INPUT);
  pinMode(ESP_RST_PIN, OUTPUT);
  digitalWrite(ESP_RST_PIN, LOW);
  delay(10);
  digitalWrite(ESP_RST_PIN, HIGH);
  delay(750);
  pinMode(ESP_RST_PIN, INPUT);

  SPI1.setRX(ESP_MISO_PIN);
  SPI1.setTX(ESP_MOSI_PIN);
  SPI1.setSCK(ESP_SCK_PIN);
  SPI1.setCS(ESP_CS_PIN);
  SPI1.begin();
}

void loop() {
  checkButtons();
  checkWandData();
}


/////////////////////////////////////////////////////////////////////

void checkForPacket() {
  int packetSize = udp.parsePacket();
  if (packetSize) {
    udp.read(packetBuffer, PACKET_BUF_SIZE);
    if (packetBuffer[0] == PACKET_ID_ROBBIE_MODE && packetSize == 2) {
      currentRobbieMode = packetBuffer[1];
      updateSegDisplay();
    } 
  }
}

void updateSegDisplay() {
  uint8_t val = seg_lookup[currentRobbieMode];
  if (numWandsConnected > 0)
    val |= 1;

  digitalWrite(SEG_DIG1_PIN, LOW);
  digitalWrite(SEG_EN_PIN, LOW);
  for (int i = 0; i < 8; i++)  {
    digitalWrite(SEG_SER_PIN, !!(val & (1 << (7 - i))));
    digitalWrite(SEG_SCK_PIN, HIGH);
    digitalWrite(SEG_SCK_PIN, LOW);
  }
  digitalWrite(SEG_EN_PIN, HIGH);
  digitalWrite(SEG_DIG1_PIN, HIGH);
}

void checkButtons() {
  static unsigned long lastButtonCheck = 0;
  static uint8_t btn1State = 0;
  static uint8_t btn2State = 0;
  if (millis() - lastButtonCheck > 50) {
    uint8_t state = 1 - digitalRead(BTN_1_PIN);
    if (btn1State == 0 && state == 1) {
      if (currentRobbieMode > 0)
      currentRobbieMode--;
      sendMode = 1;
      updateSegDisplay();
    }
    btn1State = state;

    state = 1 - digitalRead(BTN_2_PIN);
    if (btn2State == 0 && state == 1) {
      if (currentRobbieMode < 9)
      currentRobbieMode++;
      sendMode = 1;
      updateSegDisplay();
    }
    btn2State = state;

    lastButtonCheck = millis();
  }
}

void sendLaserData() {
  //TODO
}

void sendButtonData() {
  if (sendMode == 1) {
    if (udp.beginPacket(jukeboxIP, 8888) == 1) {
      uint8_t buf[2] = { PACKET_ID_JUKEBOX_MODE, MODE_MAPPING[currentRobbieMode] };
      udp.write(buf, 2);
      udp.endPacket();
    }

    if (udp.beginPacket(ioIP, 8888) == 1) {
      uint8_t buf[2] = { PACKET_ID_ROBBIE_MODE, currentRobbieMode };
      udp.write(buf, 2);
      udp.endPacket();
    }
    
    sendMode = 0;
  }
}

void slaveSelect() {
  while (digitalRead(ESP_RDY_PIN) == HIGH);
  SPI1.beginTransaction(spiSettings);
  digitalWrite(ESP_CS_PIN, LOW);
  for (unsigned long start = millis(); (digitalRead(ESP_RDY_PIN) != HIGH) && (millis() - start) < 5;);
}

void slaveDeselect() {
  digitalWrite(ESP_CS_PIN, HIGH);
  SPI1.endTransaction();
}

void checkWandData() {
  slaveSelect();
  for (int i = 0; i < 4; i++)
    SPI1.transfer(0);
  slaveDeselect();
  
  slaveSelect();
  for (int i = 0; i < 40; i++)
    spiBuffer[i] = SPI1.transfer(0);
  slaveDeselect();

  for (uint8_t i = 0; i < 4; i++) {
    wandData[i].w = (uint16_t)spiBuffer[1 + i * 9] << 8 | spiBuffer[2 + i * 9];
    wandData[i].x = (uint16_t)spiBuffer[3 + i * 9] << 8 | spiBuffer[4 + i * 9];
    wandData[i].y = (uint16_t)spiBuffer[5 + i * 9] << 8 | spiBuffer[6 + i * 9];
    wandData[i].z = (uint16_t)spiBuffer[7 + i * 9] << 8 | spiBuffer[8 + i * 9];
    wandData[i].buttonPressed = spiBuffer[9 + i * 9];
  }

  uint8_t _numWandsConnected = spiBuffer[0];
  if (_numWandsConnected != numWandsConnected) {
    numWandsConnected = _numWandsConnected;
    updateSegDisplay();
  }
}

void sendWandData() {
  static unsigned long lastUpdate = 0;
  static unsigned long extraDelay = 0;

  if (numWandsConnected == 0) return;

  if (millis() - lastUpdate > 30 + extraDelay) {
    if (udp.beginPacket(jukeboxIP, 8888) == 1) {
      uint8_t buf[10];
      buf[0] = PACKET_ID_WAND_DATA;
      buf[1] = (uint8_t)(wandData[0].w >> 8);
      buf[2] = (uint8_t)(wandData[0].w & 0xff);
      buf[3] = (uint8_t)(wandData[0].x >> 8);
      buf[4] = (uint8_t)(wandData[0].x & 0xff);
      buf[5] = (uint8_t)(wandData[0].y >> 8);
      buf[6] = (uint8_t)(wandData[0].y & 0xff);
      buf[7] = (uint8_t)(wandData[0].z >> 8);
      buf[8] = (uint8_t)(wandData[0].z & 0xff);
      buf[9] = wandData[0].buttonPressed;
      udp.write(buf, 10);
      extraDelay = udp.endPacket() == 1 ? 0 : 1000;
    } else {
      extraDelay = 1000;
    }

    lastUpdate = millis();
  }
}
