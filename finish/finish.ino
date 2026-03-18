// For a connection via I2C using the Arduino Wire include:
#include <Wire.h>               
#include "time.h"
#include "display.h"

#define BUTTON_PIN 0

Time stopWatch;
Display dp;

void IRAM_ATTR handleButtonPress() {
  stopWatch.trigger();
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
  stopWatch.start();

  dp.setFont(ArialMT_Plain_10);

}

void VextON(void)
{
  pinMode(Vext,OUTPUT);
  digitalWrite(Vext, LOW);
}

void VextOFF(void) //Vext default OFF
{
  pinMode(Vext,OUTPUT);
  digitalWrite(Vext, HIGH);
}

void loop() {

  dp.clear();

  dp.setTextAlignment(TEXT_ALIGN_LEFT);
  dp.setFont(ArialMT_Plain_16);
  dp.drawString(0, 0, "FINISH");

  dp.setTextAlignment(TEXT_ALIGN_CENTER);
  dp.setFont(ArialMT_Plain_16);    
  String out = stopWatch.draw();
  dp.drawString(64, 25, out);

  dp.setTextAlignment(TEXT_ALIGN_RIGHT);
  dp.setFont(ArialMT_Plain_10);
  dp.drawString(127, 54, String(millis() / 1000) + "s");

  dp.display();

  delay(10);

}
