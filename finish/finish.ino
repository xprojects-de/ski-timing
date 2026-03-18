// For a connection via I2C using the Arduino Wire include:
#include <Wire.h>               
#include "HT_SSD1306Wire.h"

static SSD1306Wire  display(0x3c, 500000, SDA_OLED, SCL_OLED, GEOMETRY_128_64, RST_OLED); // addr , freq , i2c group , resolution , rst

int counter = 1;
unsigned long startTime = 0;
const unsigned long limitMS = 20000;
unsigned long lastTime = 0;

void setup() {

  Serial.begin(115200);
  Serial.println();
  Serial.println();
  VextON();
  delay(100);

  // Initialising the UI will init the display too.
  display.init();

  display.setFont(ArialMT_Plain_10);

}

void xTime() {

  unsigned long currentTime = millis();
  unsigned long elapsed = currentTime - startTime;

  if (elapsed >= limitMS) {

    startTime = currentTime;
    lastTime = elapsed;
    elapsed = 0;

  }

  int seconds = elapsed / 1000;
  int hundredths = (elapsed % 1000) / 10;

  String timeString = String(seconds) + ".";
  if (hundredths < 10) timeString += "0";
  timeString += String(hundredths);

  int secondsLast = lastTime / 1000;
  int hundredthsLast = (lastTime % 1000) / 10;

  String timeStringLast = String(secondsLast) + ".";
  if (hundredthsLast < 10) timeStringLast += "0";
  timeStringLast += String(hundredthsLast);

  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.setFont(ArialMT_Plain_16);
  display.drawString(64, 20, String(timeStringLast) + " / " + String(timeString));

}

void xDemo() {

  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_16);
  display.drawString(0, 0, "FINISH");

  xTime();

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

  display.clear();
  xDemo();

  display.setTextAlignment(TEXT_ALIGN_RIGHT);
  display.setFont(ArialMT_Plain_10);
  display.drawString(127, 54, String(millis() / 1000) + "s");

  // write the buffer to the display
  display.display();

  counter++;
  delay(10);

}
