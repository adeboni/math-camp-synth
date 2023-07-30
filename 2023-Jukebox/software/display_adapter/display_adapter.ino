#include "Defines.h"
#include "ShiftRegisterPWM.h"
#include <LiquidCrystal.h>
#include <SerialTransfer.h>
#include "Queue.h"

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

ShiftRegisterPWM sr(5, 8);
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
  if (abs(val - LTL_BTN) < 0.5) return 1;
  if (abs(val - LTR_BTN) < 0.5) return 2;
  if (abs(val - NBL_BTN) < 0.5) return 3;
  if (abs(val - NBR_BTN) < 0.5) return 4;
  if (abs(val - ENT_BTN) < 0.5) return 5;
  if (abs(val - SKP_BTN) < 0.5) return 6;
  if (abs(val - PWR_BTN) < 0.5) return 7;
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
  if (songQueue.itemCount() > 6)
    lcd.print("...");

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
    lcd.print((char)playingID.letter);
    lcd.print((char)playingID.number);
    lcd.print(' ');
    
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
    if (lastSong.letter == pendingID.letter && lastSong.number == pendingID.number) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Can't add same song twice");
      delay(2000);
      return;
    }
  }

  if (songQueue.isFull()) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Song queue full");
    delay(2000);
    return;
  }
  
  songQueue.enqueue(pendingID);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Song added to queue");
  delay(2000);
      
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
    delay(500);
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
