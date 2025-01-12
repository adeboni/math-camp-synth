#include <SPI.h>
#include <Wire.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
#include <LiquidCrystal.h>
#include "MAX7313.h"

#define SDA_PIN       2
#define SCL_PIN       3
#define LCD_1_BL_PIN  4
#define LCD_2_BL_PIN  5
#define LCD_RS_PIN    9
#define LCD_2_EN_PIN  10
#define LCD_1_EN_PIN  11
#define LCD_DB4_PIN   12
#define LCD_DB5_PIN   13
#define LCD_DB6_PIN   14
#define LCD_DB7_PIN   15
#define WIZ_MISO_PIN  16
#define WIZ_CS_PIN    17
#define WIZ_SCK_PIN   18
#define WIZ_MOSI_PIN  19
#define WIZ_RST_PIN   20
#define WIZ_INT_PIN   21

#define MAX7313_ADDR0 0x20
#define MAX7313_ADDR1 0x21
#define MAX7313_ADDR2 0x23

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

LiquidCrystal lcd1(LCD_RS_PIN, LCD_1_EN_PIN, LCD_DB4_PIN, LCD_DB5_PIN, LCD_DB6_PIN, LCD_DB7_PIN);
LiquidCrystal lcd2(LCD_RS_PIN, LCD_2_EN_PIN, LCD_DB4_PIN, LCD_DB5_PIN, LCD_DB6_PIN, LCD_DB7_PIN);

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0x00, 0x31 };
IPAddress ip(10, 0, 0, 31);
EthernetUDP udp;
uint8_t packetBuffer[UDP_TX_PACKET_MAX_SIZE];

IPAddress jukeboxIP(10, 0, 0, 32);
IPAddress laserControllerIP(10, 0, 0, 33);

MAX7313 io1(MAX7313_ADDR0, &Wire1);
MAX7313 io2(MAX7313_ADDR1, &Wire1);
MAX7313 io3(MAX7313_ADDR2, &Wire1);

// MAX7313 pin numbers
uint8_t DOTS_OUT[6]    = {7, 6, 4, 2, 0, 15};        //ADDR1
uint8_t BUTTONS_OUT[7] = {14, 13, 8, 11, 10, 9, 12}; //ADDR1
uint8_t MOTORS_OUT[3]  = {5, 3, 1};                  //ADDR1
uint8_t LAMP_OUT[1]    = {8};                        //ADDR0
uint8_t MOUTH_OUT[15]  = {13, 12, 11, 10, 9, //RED     ADDR0
                          7, 5, 3, 1, 15,    //WHITE
                          6, 4, 2, 0, 14};   //BLUE
uint8_t MODE_IN[8]     = {7, 5, 3, 1, 0, 6, 4, 2};   //ADDR2
uint8_t BUTTONS_IN[7]  = {14, 13, 8, 11, 10, 9, 12}; //ADDR2

/////////////////////////////////////////////////////////////////////

void setup1() {
  pinMode(LCD_1_BL_PIN, OUTPUT);
  pinMode(LCD_2_BL_PIN, OUTPUT);
  digitalWrite(LCD_1_BL_PIN, HIGH);
  digitalWrite(LCD_2_BL_PIN, HIGH);
  lcd1.begin(40, 2);
  lcd2.begin(40, 2);
  lcd1.print("Booting...");

  Ethernet.init(WIZ_CS_PIN);
  Ethernet.begin(mac, ip);
  Ethernet.setRetransmissionCount(0);
  Ethernet.setRetransmissionTimeout(0);
  udp.begin(8888);
}

void loop1() {
  checkForPacket();
}

void setup() {
  Wire1.setSDA(SDA_PIN);
  Wire1.setSCL(SCL_PIN);
  Wire1.begin();

  io1.begin();
  io2.begin();
  io3.begin();

  for (int i = 0; i < 16; i++) {
    io1.pinMode(i, OUTPUT);
    io1.analogWrite(i, 0);
    io2.pinMode(i, OUTPUT);
    io2.analogWrite(i, 0);
    io3.pinMode(i, INPUT);
  }
}

void loop() {
  checkButtons();

  for (int i = 0; i < 16; i++) {
    io1.analogWrite(i, 15);
    delay(500);
    io1.analogWrite(i, 0);
  }

  for (int i = 0; i < 16; i++) {
    io2.analogWrite(i, 15);
    delay(500);
    io2.analogWrite(i, 0);
  }
}


/////////////////////////////////////////////////////////////////////

void checkForPacket() {
  int packetSize = udp.parsePacket();
  if (packetSize) {
    udp.read(packetBuffer, UDP_TX_PACKET_MAX_SIZE);
  }
}

void checkButtons() {
  static unsigned long lastButtonCheck = 0;
  if (millis() - lastButtonCheck > 50) {

    
    lastButtonCheck = millis();
  }
}
