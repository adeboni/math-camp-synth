#include <LiquidCrystal.h>
#include <SerialTransfer.h>

#define NES_DAT_PIN 10
#define NES_CLK_PIN 8
#define NES_LTC_PIN 9

LiquidCrystal lcd(12, 11, 5, 4, 3, 2);
SerialTransfer uartTransfer;

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
  pinMode(NES_DAT_PIN, INPUT);
  pinMode(NES_CLK_PIN, OUTPUT);
  pinMode(NES_LTC_PIN, OUTPUT);
  digitalWrite(NES_CLK_PIN, LOW);
  digitalWrite(NES_LTC_PIN, LOW);
  
  lcd.begin(40, 2);
  lcd.clear();

  Serial.begin(115200);
  uartTransfer.begin(Serial);
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
}
