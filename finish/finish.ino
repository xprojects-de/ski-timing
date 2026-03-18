// For a connection via I2C using the Arduino Wire include:
#include <Wire.h>
#include "stopwatch.h"
#include "display.h"
#include "lora.h"
#include "accesspoint.h"

#define BUTTON_PIN 0

StopWatch stopWatch;
Display dp;
Lora lora;
AccessPoint accessPoint;

void IRAM_ATTR handleButtonPress() {
  stopWatch.trigger();
}

void showChipId() {

  // Licence
  // https://resource.heltec.cn/search
  uint64_t chipId = ESP.getEfuseMac();
  Serial.printf("ESP32 Chip model = %s Rev %d\n", ESP.getChipModel(), ESP.getChipRevision());
  Serial.printf("This chip has %d cores\n", ESP.getChipCores());
  Serial.printf("ESP32ChipID=%04X", (uint16_t)(chipId >> 32));
  Serial.printf("%08X\r\n", (uint32_t)chipId);
}

void setup() {

  Serial.begin(115200);
  Serial.println();
  Serial.println();

  showChipId();

  lora.preInit();

  VextON();
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  attachInterrupt(BUTTON_PIN, handleButtonPress, FALLING);

  delay(100);

  dp.init();
  lora.init();
  accessPoint.init();

  dp.setFont(ArialMT_Plain_10);
}

void VextON(void) {
  pinMode(Vext, OUTPUT);
  digitalWrite(Vext, LOW);
}

void VextOFF(void)  //Vext default OFF
{
  pinMode(Vext, OUTPUT);
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

  lora.proccess();
  if (lora.receive() == true) {
    stopWatch.trigger();
  }

  delay(10);
}
