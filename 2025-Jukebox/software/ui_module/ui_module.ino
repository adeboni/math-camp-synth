#include <SPI.h>
#include <Wire.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
#include <LiquidCrystal.h>
#include "MAX7313.h"
#include "Animation.h"

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
#define AIN_PIN       26

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
IPAddress jukeboxIP(10, 0, 0, 32);
IPAddress laserControllerIP(10, 0, 0, 33);
EthernetUDP udp;
#define PACKET_BUF_SIZE 1472
uint8_t packetBuffer[PACKET_BUF_SIZE];

uint16_t prevSelectedSongIndex = 1000;
uint16_t prevPlayingSongIndex = 1000;
uint8_t prevSongQueueLength = 255;
bool updateDisplay = false;

#define MAX_SONG_NAME_LEN 60
uint8_t buttonsToSend[7] = { 0, 0, 0, 0, 0, 0, 0 };
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

String MODE_NAMES[10] = { 
  "Invalid Mode",
  "Jukebox", 
  "Audio Visualization", 
  "Equations", 
  "Spirograph",
  "Pong",
  "Robbie",
  "Wand Drawing",
  "Wand Synth",
  "Calibration"
};

#define NUM_SOUND_EFFECTS 11
char soundEffects[NUM_SOUND_EFFECTS][15] = {
  "/robbie/1.wav",
  "/robbie/2.wav",
  "/robbie/3.wav",
  "/robbie/4.wav",
  "/robbie/5.wav",
  "/robbie/6.wav",
  "/robbie/7.wav",
  "/robbie/8.wav",
  "/robbie/9.wav",
  "/robbie/10.wav",
  "/robbie/11.wav"
};
unsigned long nextSoundEffectTime = 0;

MAX7313 io1(MAX7313_ADDR0, &Wire1);
MAX7313 io2(MAX7313_ADDR1, &Wire1);
MAX7313 io3(MAX7313_ADDR2, &Wire1);

// MAX7313 pin numbers
uint8_t DOTS_OUT[6]    = {7, 6, 4, 2, 0, 15};        //ADDR1
uint8_t BUTTONS_OUT[7] = {8, 9, 10, 11, 12, 13, 14}; //ADDR1
uint8_t MOTORS_OUT[3]  = {5, 3, 1};                  //ADDR1
uint8_t LAMP_OUT[1]    = {8};                        //ADDR0
uint8_t MOUTH_OUT[15]  = {13, 12, 11, 10, 9, //RED     ADDR0
                          7, 5, 3, 1, 15,    //WHITE
                          6, 4, 2, 0, 14};   //BLUE
uint8_t MODE_IN[8]     = {7, 5, 3, 1, 0, 6, 4, 2};   //ADDR2
uint8_t MODE_VAL[8]    = {8, 7, 6, 5, 4, 3, 2, 1};
uint8_t BUTTONS_IN[7]  = {14, 13, 12, 11, 10, 9, 8}; //ADDR2
uint8_t BUTTON_VAL[7]  = {0, 1, 2, 3, 4, 5, 6};

Animation ani(&io1, &io2, DOTS_OUT, MOTORS_OUT, LAMP_OUT, MOUTH_OUT);

/////////////////////////////////////////////////////////////////////

void setup1() {
  pinMode(LCD_1_BL_PIN, OUTPUT);
  pinMode(LCD_2_BL_PIN, OUTPUT);
  digitalWrite(LCD_1_BL_PIN, HIGH);
  digitalWrite(LCD_2_BL_PIN, HIGH);
  lcd1.begin(40, 2);
  lcd2.begin(40, 2);
  lcd1.clear();
  lcd2.clear();

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
  sendUDPButtons();
  sendSoundEffect();
}

void setup() {
  pinMode(AIN_PIN, INPUT);
  randomSeed(analogRead(AIN_PIN));

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
    io2.analogWrite(i, 15);
    io3.pinMode(i, INPUT);
  }

  delay(500);
  for (int i = 0; i < 3; i++)
    io2.analogWrite(MOTORS_OUT[i], 0);

  for (int i = 0; i < 6; i++) {
    io2.analogWrite(DOTS_OUT[i], 0);
    delay(100);
    io2.analogWrite(DOTS_OUT[i], 15);
  }

  for (int i = 0; i < 7; i++) {
    io2.analogWrite(BUTTONS_OUT[i], 0);
    delay(100);
    io2.analogWrite(BUTTONS_OUT[i], 15);
  }

  for (int i = 0; i < 1; i++) {
    io1.analogWrite(LAMP_OUT[i], 15);
    delay(100);
    io1.analogWrite(LAMP_OUT[i], 0);
  }

  for (int i = 0; i < 15; i++) {
    io1.analogWrite(MOUTH_OUT[i], 15);
    delay(100);
    io1.analogWrite(MOUTH_OUT[i], 0);
  }

  ani.start();
}

void loop() {
  checkButtons();
  ani.update();
}


/////////////////////////////////////////////////////////////////////

void checkForPacket() {
  int packetSize = udp.parsePacket();
  if (packetSize) {
    udp.read(packetBuffer, PACKET_BUF_SIZE);
    if (packetBuffer[0] == PACKET_ID_ROBBIE_MODE && packetSize == 2) {
      currentRobbieMode = packetBuffer[1];
      updateDisplay = true;
    } else if (packetBuffer[0] == PACKET_ID_AUDIO_METADATA) {
      uint16_t selectedSongIndex = ((uint16_t)packetBuffer[1] << 8) | (uint16_t)packetBuffer[2];
      uint16_t playingSongIndex = ((uint16_t)packetBuffer[3] << 8) | (uint16_t)packetBuffer[4];
      uint8_t songQueueLength = packetBuffer[5];

      if (selectedSongIndex != prevSelectedSongIndex || playingSongIndex != prevPlayingSongIndex ||
          songQueueLength != prevSongQueueLength || updateDisplay) {
      
        prevSelectedSongIndex = selectedSongIndex;
        prevPlayingSongIndex = playingSongIndex;
        prevSongQueueLength = songQueueLength;
        updateDisplay = false;

        char currentSong[MAX_SONG_NAME_LEN];
        for (int i = 0; i < MAX_SONG_NAME_LEN; i++)
          currentSong[i] = packetBuffer[6 + songQueueLength * 2 + i];

        lcd1.clear();
        lcd1.setCursor(0, 0);
        lcd1.print("Playing: ");
        lcd1.print(currentSong[0] == 0 ? "None" : currentSong);

        lcd1.setCursor(0, 1);
        lcd1.print("Queue: ");
    
        for (uint8_t i = 0; i < songQueueLength; i++) {
          uint16_t songIndex = ((uint16_t)packetBuffer[6 + i * 2] << 8) | (uint16_t)packetBuffer[6 + i * 2 + 1];
          lcd1.print((char)((songIndex / 10) + 65));
          lcd1.print((char)((songIndex % 10) + 48));
          lcd1.print(" ");
          if (i == 8 && songQueueLength > 9) {
            lcd1.print("...");
            break;
          }
        }

        lcd2.clear();
        lcd2.setCursor(0, 0);
        lcd2.print("Mode: ");
        lcd2.print(currentRobbieMode);
        lcd2.print(" - ");
        lcd2.print(MODE_NAMES[currentRobbieMode]);

        lcd2.setCursor(0, 1);
        lcd2.print("Song Selection: ");
        lcd2.print((char)((selectedSongIndex / 10) + 65));
        lcd2.print((char)((selectedSongIndex % 10) + 48));
      }
    }
  }
}

void checkButtons() {
  static unsigned long lastButtonCheck = 0;
  static uint8_t mode_states[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
  static uint8_t button_states[7] = { 0, 0, 0, 0, 0, 0, 0 };

  if (millis() - lastButtonCheck > 50) {
    for (int i = 0; i < 8; i++) {
      uint8_t state = 1 - io3.digitalRead(MODE_IN[i]);
      if (mode_states[i] == 0 && state == 1) {
        currentRobbieMode = MODE_VAL[i];
        updateDisplay = true;
        sendMode = 1;
        nextSoundEffectTime = millis() + 5000;
      }
      mode_states[i] = state;
    }
    
    for (int i = 0; i < 7; i++) {
      uint8_t state = 1 - io3.digitalRead(BUTTONS_IN[i]);
      io2.analogWrite(BUTTONS_OUT[i], state == 0 ? 15 : 0);
      if (button_states[i] == 0 && state == 1) {
        buttonsToSend[i] = 1;

        if (currentRobbieMode == 6) {
          switch (i) {
            case 0:
              ani.forceAnimation(AM_DOTS_NIGHTRIDER);
              break;
            case 1:
              ani.forceAnimation(AM_MOUTH_PULSE);
              break;
            case 2:
              ani.forceAnimation(AM_MOTORS_SPIN);
              break;
            case 3:
              ani.forceAnimation(AM_LAMP_MORSE_CODE);
              break;
            default:
              break;
          }
        }
      }
      button_states[i] = state;
    }
    
    lastButtonCheck = millis();
  }
}

void sendUDPButtons() {
  if (sendMode == 1) {
    if (udp.beginPacket(laserControllerIP, 8888) == 1) {
      uint8_t buf[2] = { PACKET_ID_ROBBIE_MODE, currentRobbieMode };
      udp.write(buf, 2);
      udp.endPacket();
    }

    if (udp.beginPacket(jukeboxIP, 8888) == 1) {
      uint8_t buf[2] = { PACKET_ID_JUKEBOX_MODE, MODE_MAPPING[currentRobbieMode] };
      udp.write(buf, 2);
      udp.endPacket();
    }

    sendMode = 0;
  }

  for (int i = 0; i < 7; i++) {
    if (buttonsToSend[i] == 1) {
      buttonsToSend[i] = 0;
      if (udp.beginPacket(jukeboxIP, 8888) == 1) {
        uint8_t buf[2] = { PACKET_ID_BUTTON_PRESS, BUTTON_VAL[i] };
        udp.write(buf, 2);
        udp.endPacket();
      }
    }
  }
}

void sendSoundEffect() {
  if (currentRobbieMode != 6) return;
  if (millis() > nextSoundEffectTime) {
    nextSoundEffectTime = millis() + 60000;
    int index = random(NUM_SOUND_EFFECTS);
    
    if (udp.beginPacket(jukeboxIP, 8888) == 1) {
      uint8_t buf[20];
      buf[0] = PACKET_ID_PLAY_EFFECT;
      for (int i = 0; i < strlen(soundEffects[index]); i++)
        buf[1 + i] = (uint8_t)(soundEffects[index][i]);
      buf[1 + strlen(soundEffects[index])] = 0;
      
      udp.write(buf, 2 + strlen(soundEffects[index]));
      udp.endPacket();
    }
  }
}
