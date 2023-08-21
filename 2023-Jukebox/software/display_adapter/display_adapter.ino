#include "Defines.h"
#include "ShiftRegisterPWM.h"
#include <LiquidCrystal.h>
#include <SerialTransfer.h>
#include "Queue.h"

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
const char maxLetter = 'U';
int refVoltageMode = 0;
int lastMode = 0;
int refVoltageButtons = 0;
int lastButton = 0;
ArduinoQueue<songID> songQueue(50);
unsigned long lastUpdate = 0;
int songTitleIndex = 0;
unsigned long animationTimeLeft[4] = {0};
unsigned long timeToPowerOff = POWER_TIMEOUT_MS;

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

void lightButtons(int pressedButton) {
  for (int i = 0; i < 7; i++)
    sr.set(BTN_LED_ARRAY[i], pressedButton == i + 1 ? 255 : (timeToPowerOff > 0 ? 32 : 0));
}

int getButton() {
  float val = (100.0 * analogRead(BTN_PIN)) / refVoltageButtons;
  if (abs(val - LTL_BTN) < 0.5) return 1;
  if (abs(val - LTR_BTN) < 0.5) return 2;
  if (abs(val - NBL_BTN) < 0.5) return 3;
  if (abs(val - NBR_BTN) < 0.5) return 4;
  if (abs(val - ENT_BTN) < 0.5) return 5;
  if (abs(val - SKP_BTN) < 0.5) return 6;
  if (abs(val - PWR_BTN) < 0.5) return 7;
  return 0;
}

int getMode() { 
  float val = (100.0 * analogRead(MODE_PIN)) / refVoltageMode;
  if (abs(val - MODE_0) < 1) return 0;
  if (abs(val - MODE_1) < 1) return 1;
  if (abs(val - MODE_2) < 1) return 2;
  if (abs(val - MODE_3) < 1) return 3;
  if (abs(val - MODE_4) < 1) return 4;
  if (abs(val - MODE_5) < 1) return 5;
  if (abs(val - MODE_6) < 1) return 6;
  if (abs(val - MODE_7) < 1) return 7;
  return 255;
}

void updateDisplay() {
  lcd.clear();
  if (timeToPowerOff == 0)
    return;
    
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

  if (lastMode == 0) {
    lcd.setCursor(31, 0);
    lcd.print("Input: ");
    lcd.print((char)pendingID.letter);
    lcd.print((char)pendingID.number);
  }

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

  int sumButtons = 0;
  int sumMode = 0;
  for (int i = 0; i < 5; i++) {
    sumButtons += analogRead(BTN_PIN);
    sumMode += analogRead(MODE_PIN);
    delay(200);
  }
  refVoltageButtons = sumButtons / 5;
  refVoltageMode = sumMode / 5;

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

void playRobbieSounds() {
  if (playingID.letter != '-')
    return;
    
  playingID = {'U', '0'};
  rxStruct.songLength = 600000;
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

int Cube255(float value) {
  return constrain((int)(value * value * value / 255.0 / 255.0), 0, 255);
}

void updateAnimations(int startAnimation = -1) {
  static unsigned long lastAnimationUpdate = 0;

  if (startAnimation >= 4)
    return;
    
  if (startAnimation >= 0)
    animationTimeLeft[startAnimation] = ANIMATION_LENGTH_MS;

  float mouthColorIndex = sin(millis() / 20 * PI / 180) + 1;
  int mouthRedLevel = Cube255(255.0 - 85.0 * abs(0 - mouthColorIndex));
  int mouthBlueLevel = Cube255(255.0 - 85.0 * abs(1 - mouthColorIndex));
  int mouthWhiteLevel = Cube255(255.0 - 85.0 * abs(2 - mouthColorIndex));

  float dotIndex = sin(millis() / 5 * PI / 180) * 5.0 + 2.5;
  int lampCode[] = {1, 1, 1, 0, 1, 1, 1, 0, 0, 1, 0, 1, 1, 1, 0, 0, 1, 1, 1, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 0};
  int lampCodeIndex = millis() / 400 % (sizeof(lampCode) / sizeof(int));

  int motor3Code[] = {0, 0, 1, 0, 1, 1, 0, 0, 0, 1};
  int motor2Code[] = {0, 1, 0, 1, 0, 1, 0, 0, 1, 0};
  int motor1Code[] = {1, 0, 1, 1, 0, 1, 0, 1, 0, 0};
  int motorCodeIndex = millis() / 2000 % 10;

  unsigned long newTime = millis();
  for (int i = 0; i < 4; i++) {
    int animationStage = i + ((animationTimeLeft[i] >= ANIMATION_LENGTH_MS / 2) ? 10 : 0); 
    if (animationTimeLeft[i] > 0) {
      if (lastMode == 1)
        playRobbieSounds();
      
      switch (animationStage) {
        case 0:
          for (uint8_t i = 0; i < 5; i++)
            sr.set(MOUTH_R_ARRAY[i], (uint8_t)(((float)sin(millis() / 150.0) + 1) * (3 - abs(i % 5 - 2)) / 3.0 * 128 * mouthRedLevel / 255));
          for (uint8_t i = 0; i < 5; i++)
            sr.set(MOUTH_B_ARRAY[i], (uint8_t)(((float)sin(millis() / 150.0) + 1) * (3 - abs(i % 5 - 2)) / 3.0 * 128 * mouthBlueLevel / 255));
          for (uint8_t i = 0; i < 5; i++)
            sr.set(MOUTH_W_ARRAY[i], (uint8_t)(((float)sin(millis() / 150.0) + 1) * (3 - abs(i % 5 - 2)) / 3.0 * 16 * mouthWhiteLevel / 255));
          break;
        case 10:
          for (uint8_t i = 0; i < 5; i++)
            sr.set(MOUTH_R_ARRAY[i], (uint8_t)(((float)sin(millis() / 150.0) + 1) * 128 * mouthRedLevel / 255));
          for (uint8_t i = 0; i < 5; i++)
            sr.set(MOUTH_B_ARRAY[i], (uint8_t)(((float)sin(millis() / 150.0) + 1) * 128 * mouthBlueLevel / 255));
          for (uint8_t i = 0; i < 5; i++)
            sr.set(MOUTH_W_ARRAY[i], (uint8_t)(((float)sin(millis() / 150.0) + 1) * 16 * mouthWhiteLevel / 255));
          break;
        case 1:
          for (int i = 0; i < 6; i++) {
            float level = 255.0 - 51.0 * abs(i - dotIndex);
            sr.set(DOT_ARRAY[i], Cube255(level));
          }
          break;
        case 11:
          for (int i = 0; i < 6; i++)
            sr.set(DOT_ARRAY[i], (uint8_t)(((float)sin(millis() / 150.0) + 1) * 128));
          break;
        case 2:
        case 12:
          sr.set(MOTOR_0, motor1Code[motorCodeIndex] * 255);
          sr.set(MOTOR_1, motor2Code[motorCodeIndex] * 255);
          sr.set(MOTOR_2, motor3Code[motorCodeIndex] * 255);
          break;
        case 3:
          sr.set(LAMP, lampCode[lampCodeIndex] * 255);
          break;
        case 13:
          sr.set(LAMP, (uint8_t)(((float)sin(millis() / 150.0) + 1) * 128));
          break;
      }
      
      animationTimeLeft[i] -= min(animationTimeLeft[i], newTime - lastAnimationUpdate);
      
      if (animationTimeLeft[i] == 0) {
        switch (i) {
          case 0:
            for (uint8_t i = 0; i < 15; i++)
              sr.set(MOUTH_ARRAY[i], 0);
            break;
          case 1:
            for (uint8_t i = 0; i < 6; i++)
              sr.set(DOT_ARRAY[i], 0);  
            break;
          case 2:
            sr.set(MOTOR_0, 0);
            sr.set(MOTOR_1, 0);
            sr.set(MOTOR_2, 0);
            break;
          case 3:
            sr.set(LAMP, 0);
            break;
        }
      }
    }
  }

  lastAnimationUpdate = newTime;
}

void loop() {
  static unsigned long lastRandomAnimation = millis();
  static unsigned long lastButtonPress = 0;
  static unsigned long lastPowerOffTimeCheck = millis();

  int mode = getMode();
  if (mode != 255 && mode != lastMode) {
    lastMode = mode;
    timeToPowerOff = POWER_TIMEOUT_MS;
  }
  
  unsigned long newButtonPress = millis();
  int button = getButton();
  lightButtons(button);

  if (button != lastButton && button != 0 && newButtonPress - lastButtonPress > 300) {
    if (button > 0 && button < 7)
      timeToPowerOff = POWER_TIMEOUT_MS;
    else if (button == 7) {
      if (timeToPowerOff > POWER_GRACE_MS)
        timeToPowerOff = POWER_GRACE_MS;
      else
        timeToPowerOff = POWER_TIMEOUT_MS;
    }
    
    if (lastMode == 0) {   
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
    } else if (lastMode == 1) {
      updateAnimations(button - 1);
    }
    
    updateDisplay();
    lastButtonPress = newButtonPress;
  }

  lastButton = button;
  
  if (uartTransfer.available()) {
    uartTransfer.rxObj(rxStruct);
    updateDisplay();
  }

  if (millis() - lastUpdate > 1000)
    updateDisplay();

  if (millis() - lastRandomAnimation > 120000 && lastMode == 0 && timeToPowerOff > 0) {
    updateAnimations(random(4));
    lastRandomAnimation = millis();
  }
  updateAnimations();

  if (rxStruct.songLength == 0 && playingID.letter != '-')
    playNextSong();

  unsigned long newPowerOffTimeCheck = millis();
  timeToPowerOff -= min(newPowerOffTimeCheck - lastPowerOffTimeCheck, timeToPowerOff);
  lastPowerOffTimeCheck = newPowerOffTimeCheck;
  if (timeToPowerOff == 0) {
    if (playingID.letter != '-') {
      while (!songQueue.isEmpty())
        songQueue.dequeue();
      playNextSong();
    }
  }
}
