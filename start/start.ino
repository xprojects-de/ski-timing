// For a connection via I2C using the Arduino Wire include:
#include <Wire.h>               
#include "display.h"

#define BUTTON_PIN 0

Display dp;

unsigned long currentMillis = 0;

void IRAM_ATTR handleButtonPress() {
  currentMillis = millis();
}

void setup() {

  Serial.begin(115200);
  Serial.println();
  Serial.println();
  VextON();
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  attachInterrupt(BUTTON_PIN, handleButtonPress, FALLING);

  delay(100);

  dp.init();

  dp.setFont(ArialMT_Plain_10);

}

void VextON(void)
{
  pinMode(Vext,OUTPUT);
  digitalWrite(Vext, LOW);
}

void VextOFF(void)
{
  pinMode(Vext,OUTPUT);
  digitalWrite(Vext, HIGH);
}

void loop() {

  dp.clear();

  dp.setTextAlignment(TEXT_ALIGN_LEFT);
  dp.setFont(ArialMT_Plain_16);
  dp.drawString(0, 0, "START");

  dp.setTextAlignment(TEXT_ALIGN_CENTER);
  dp.setFont(ArialMT_Plain_16);    
  dp.drawString(64, 25, String(currentMillis));

  dp.display();

  delay(10);

}
