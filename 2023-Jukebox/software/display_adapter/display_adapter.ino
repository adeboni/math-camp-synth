#define ShiftRegisterPWM_LATCH_PORT PORTB
#define ShiftRegisterPWM_LATCH_MASK 0B00000001
#define ShiftRegisterPWM_DATA_PORT PORTB
#define ShiftRegisterPWM_DATA_MASK 0B00000100
#define ShiftRegisterPWM_CLOCK_PORT PORTB
#define ShiftRegisterPWM_CLOCK_MASK 0B00000010
#include "ShiftRegisterPWM.h"
ShiftRegisterPWM sr(1, 255);

#include <LiquidCrystal.h>
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

#include <SerialTransfer.h>
SerialTransfer uartTransfer;

#define NES_DAT_PIN 13
#define NES_CLK_PIN 7
#define NES_LTC_PIN 6

#define LED_DAT_PIN 10
#define LED_CLK_PIN 9
#define LED_LTC_PIN 8

struct {
  char line1[41];
  char line2[41];
} rxStruct;

struct {
  uint8_t nes;
} txStruct;

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

void setup() {
  pinMode(LED_DAT_PIN, OUTPUT);
  pinMode(LED_CLK_PIN, OUTPUT);
  pinMode(LED_LTC_PIN, OUTPUT);
  
  pinMode(NES_DAT_PIN, INPUT);
  pinMode(NES_CLK_PIN, OUTPUT);
  pinMode(NES_LTC_PIN, OUTPUT);
  digitalWrite(NES_CLK_PIN, LOW);
  digitalWrite(NES_LTC_PIN, LOW);
  
  lcd.begin(40, 2);
  lcd.clear();

  Serial.begin(115200);
  uartTransfer.begin(Serial);

  sr.interrupt(ShiftRegisterPWM::UpdateFrequency::Fast);
}

void loop() {
  uint8_t nesVal = readNesController();
  if (txStruct.nes != nesVal) {
    txStruct.nes = nesVal;
    uartTransfer.sendDatum(txStruct);
  }

  if (uartTransfer.available()) {
    uartTransfer.rxObj(rxStruct);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(rxStruct.line1);
    lcd.setCursor(0, 1);
    lcd.print(rxStruct.line2);
  }

  for (uint8_t i = 0; i < 6; i++) {
    uint8_t val = (uint8_t)(((float)sin(millis() / 150.0 + i / 6.0 * 2.0 * PI) + 1) * 128);
    sr.set(i + 2, val);
  }
}
