#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUdp.h>

#define BTN_1_PIN     8
#define BTN_2_PIN     9
#define ESP_SCK_PIN   10
#define ESP_MOSI_PIN  11
#define ESP_MISO_PIN  12
#define ESP_CS_PIN    13
#define ESP_RST_PIN   14
#define ESP_BUSY_PIN  15
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
uint8_t packetBuffer[UDP_TX_PACKET_MAX_SIZE];
IPAddress ioIP(10, 0, 0, 31);
IPAddress jukeboxIP(10, 0, 0, 32);

typedef struct {
    uint16_t yaw, pitch, rotation;
} wand_data_t;

//TODO: support multiple wands
wand_data_t wandData = {0, 0, 0};

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

uint8_t numWandsConnected = 0;
const uint8_t seg_lookup[10] = {
  //pcdegfab
  0b01110111, //0
  0b01000001, //1
  0b00111011, //2
  0b01101011, //3
  0b01001101, //4
  0b01101110, //5
  0b01111110, //6
  0b01000011, //7
  0b01111111, //8
  0b01101111, //9
};

/////////////////////////////////////////////////////////////////////

void setup1() {
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
  pinMode(BTN_1_PIN, INPUT_PULLUP);
  pinMode(BTN_2_PIN, INPUT_PULLUP);
  pinMode(SEG_DIG1_PIN, OUTPUT);
  pinMode(SEG_SER_PIN, OUTPUT);
  pinMode(SEG_SCK_PIN, OUTPUT);
  pinMode(SEG_EN_PIN, OUTPUT);
  digitalWrite(SEG_DIG1_PIN, LOW);

  pinMode(ESP_BUSY_PIN, INPUT);
  pinMode(ESP_RST_PIN, OUTPUT);
  digitalWrite(ESP_RST_PIN, HIGH);
  delay(50);
  digitalWrite(ESP_RST_PIN, LOW);
  pinMode(ESP_CS_PIN, OUTPUT);
  digitalWrite(ESP_CS_PIN, HIGH);

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
    udp.read(packetBuffer, UDP_TX_PACKET_MAX_SIZE);
    if (packetBuffer[0] == PACKET_ID_ROBBIE_MODE && packetSize == 2) {
      currentRobbieMode = packetBuffer[1];
      updateSegDisplay();
    } 
  }
}

void updateSegDisplay() {
  uint8_t val = seg_lookup[currentRobbieMode];
  if (numWandsConnected > 0)
    val |= 0b10000000;

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

void checkWandData() {
  if (digitalRead(ESP_BUSY_PIN) == HIGH) {
    uint8_t buf[6];
    digitalWrite(ESP_CS_PIN, LOW);
    for (int i = 0; i < 6; i++)
      buf[i] = SPI.transfer(0x00);
    digitalWrite(ESP_CS_PIN, HIGH);

    wandData.yaw = (uint16_t)buf[0] << 8 | buf[1];
    wandData.pitch = (uint16_t)buf[2] << 8 | buf[3];
    wandData.rotation = (uint16_t)buf[4] << 8 | buf[5];
  }
}

void sendWandData() {
  //TODO
}