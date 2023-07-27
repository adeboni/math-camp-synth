#define ShiftRegisterPWM_LATCH_PORT PORTB
#define ShiftRegisterPWM_LATCH_MASK 0B00000001
#define ShiftRegisterPWM_DATA_PORT PORTB
#define ShiftRegisterPWM_DATA_MASK 0B00000100
#define ShiftRegisterPWM_CLOCK_PORT PORTB
#define ShiftRegisterPWM_CLOCK_MASK 0B00000010
#include "ShiftRegisterPWM.h"
ShiftRegisterPWM sr(5, 8);

#include <LiquidCrystal.h>
#include <SerialTransfer.h>
#include "Queue.h"

#define LED_DAT_PIN 10
#define LED_CLK_PIN 9
#define LED_LTC_PIN 8

#define NES_DAT_PIN 13
#define NES_CLK_PIN 7
#define NES_LTC_PIN 6

#define BTN_PIN  A7

#define PWR_BTN 87.5
#define SKP_BTN 85.7
#define LTL_BTN 80.0
#define LTR_BTN 50.0
#define NBL_BTN 83.4
#define NBR_BTN 66.7
#define ENT_BTN 75.0

#define PWR_BTN_LED 7
#define SKP_BTN_LED 6
#define LTL_BTN_LED 4
#define LTR_BTN_LED 1
#define NBL_BTN_LED 5
#define NBR_BTN_LED 2
#define ENT_BTN_LED 3

#define MOTOR_0    13
#define MOTOR_1    14
#define MOTOR_2    15
#define LAMP       12

#define DOT_0      39
#define DOT_1      38
#define DOT_2      37
#define DOT_3      36
#define DOT_4      35
#define DOT_5      34

#define MOUTH_B_0  21
#define MOUTH_R_0  22
#define MOUTH_W_0  23

#define MOUTH_B_1  18
#define MOUTH_R_1  19
#define MOUTH_W_1  20

#define MOUTH_B_2  31
#define MOUTH_R_2  16
#define MOUTH_W_2  17

#define MOUTH_B_3  28
#define MOUTH_R_3  29
#define MOUTH_W_3  30

#define MOUTH_B_4  25
#define MOUTH_R_4  26
#define MOUTH_W_4  27

uint8_t readNesController() {  
  uint8_t tempData = 255;

  digitalWrite(NES_LTC_PIN, HIGH);
  delayMicroseconds(12);
  digitalWrite(NES_LTC_PIN, LOW);

  for (int i = 0; i < 8; i++) {
    if (digitalRead(NES_DAT_PIN) == 0)
      bitClear(tempData, i);
    digitalWrite(NES_CLK_PIN, HIGH);
    delayMicroseconds(6);
    digitalWrite(NES_CLK_PIN, LOW);
    delayMicroseconds(6);
  }

  return tempData;
}

struct {
  uint32_t songLength;
  uint8_t songName[20];
} rxStruct;

struct songID {
  uint8_t letter;
  uint8_t number;
};

songID pendingID = {'A', '0'};
songID playingID = {'-', '-'};

LiquidCrystal lcd(12, 11, 5, 4, 3, 2);
SerialTransfer uartTransfer;

const char minLetter = 'A';
const char maxLetter = 'Z';
int refVoltage = 0;
int lastButton = 0;
ArduinoQueue<songID> songQueue(50);
unsigned long lastUpdate = 0;
int songTitleIndex = 0;

int getButton() {
  float val = (100.0 * analogRead(BTN_PIN)) / refVoltage;
  if (abs(val - LTL_BTN) < 1) return 1;
  if (abs(val - LTR_BTN) < 1) return 2;
  if (abs(val - NBL_BTN) < 1) return 3;
  if (abs(val - NBR_BTN) < 1) return 4;
  if (abs(val - ENT_BTN) < 1) return 5;
  if (abs(val - SKP_BTN) < 1) return 6;
  if (abs(val - PWR_BTN) < 1) return 7;
  return 0;
}

void updateDisplay() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Queue: ");
  for (int i = 0; i < min(songQueue.itemCount(), 6); i++) {
    songID item = songQueue.itemAt(i);
    lcd.print((char)item.letter);
    lcd.print((char)item.number);
    lcd.print(' ');
  }

  lcd.setCursor(31, 0);
  lcd.print("Input: ");
  lcd.print((char)pendingID.letter);
  lcd.print((char)pendingID.number);

  lcd.setCursor(0, 1);
  lcd.print("Playing: ");

  unsigned long newTime = millis();
  
  if (playingID.letter == '-') {
    lcd.print("--");
  } else {
    for (int i = 0; i < 20; i++) {
      if (rxStruct.songName[i] == '\0')
        break;
      lcd.print((char)rxStruct.songName[i]);
    }
  
    rxStruct.songLength -= min(newTime - lastUpdate, rxStruct.songLength);  
    char buf[10];
    unsigned long seconds = rxStruct.songLength / 1000;
    sprintf(buf, "%d:%02d", (int)(seconds / 60), (int)(seconds % 60));
    lcd.setCursor(40 - strlen(buf), 1);
    lcd.print(buf);
  }

  lastUpdate = newTime;
}

void setup() {
  rxStruct.songLength = 0;

  /*
  pinMode(NES_DAT_PIN, INPUT);
  pinMode(NES_CLK_PIN, OUTPUT);
  pinMode(NES_LTC_PIN, OUTPUT);
  digitalWrite(NES_CLK_PIN, LOW);
  digitalWrite(NES_LTC_PIN, LOW);
   */
  
  pinMode(LED_DAT_PIN, OUTPUT);
  pinMode(LED_CLK_PIN, OUTPUT);
  pinMode(LED_LTC_PIN, OUTPUT);

  lcd.begin(40, 2);
  lcd.clear();
  lcd.print("Calibrating...");
  delay(1000);

  int sum = 0;
  for (int i = 0; i < 5; i++) {
    sum += analogRead(BTN_PIN);
    delay(200);
  }
  refVoltage = sum / 5;

  lcd.clear();
  sr.interrupt(ShiftRegisterPWM::UpdateFrequency::VerySlow);
  for (int i = 0; i < 8 * 5; i++) sr.set(i, 0); 

  Serial.begin(9600);
  uartTransfer.begin(Serial);
  updateDisplay();
}

void playNextSong() {
  if (songQueue.isEmpty()) {
    playingID = {'-', '-'};
    rxStruct.songLength = 0;
  } else {
    playingID = songQueue.dequeue();
    rxStruct.songLength = 600000;
  }
  uartTransfer.sendDatum(playingID);
}

void addSongToQueue() {
  if (!songQueue.isEmpty()) {
    songID lastSong = songQueue.getTail();
    if (lastSong.letter == pendingID.letter && lastSong.number == pendingID.number)
      return;
  }
  
  songQueue.enqueue(pendingID);
  if (playingID.letter == '-')
    playNextSong();
}

void loop() {
  int button = getButton();
  if (button != lastButton && button != 0) {
    switch (button) {
      case 1:
        pendingID.letter = constrain(pendingID.letter - 1, (byte)minLetter, (byte)maxLetter);
        break;
      case 2:
        pendingID.letter = constrain(pendingID.letter + 1, (byte)minLetter, (byte)maxLetter);
        break;
      case 3:
        pendingID.number = constrain(pendingID.number - 1, (byte)'0', (byte)'9');
        break;
      case 4:
        pendingID.number = constrain(pendingID.number + 1, (byte)'0', (byte)'9');
        break;
      case 5:
        addSongToQueue();
        break;
      case 6:
        playNextSong();
        break;
    }
    
    updateDisplay();
  }

  lastButton = button;
  
  if (uartTransfer.available()) {
    uartTransfer.rxObj(rxStruct);
    updateDisplay();
  }

  if (millis() - lastUpdate > 1000)
    updateDisplay();

  if (rxStruct.songLength == 0 && playingID.letter != '-')
    playNextSong();
}