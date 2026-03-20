// For a connection via I2C using the Arduino Wire include:
#include <Wire.h>
#include "display.h"
#include "lora.h"

#define BUTTON_PIN 0

Display dp;
Lora lora;

volatile bool buttonTriggered = false;
char timeTrigger[30];

void IRAM_ATTR handleButtonPress() {
  snprintf(timeTrigger, sizeof(timeTrigger), "%lu", micros());
  buttonTriggered = true;
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

  delay(100);

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  attachInterrupt(BUTTON_PIN, handleButtonPress, FALLING);

  dp.init();
  lora.init();

  snprintf(timeTrigger, sizeof(timeTrigger), "-");

  dp.setFont(ArialMT_Plain_10);
}

void VextON(void) {
  pinMode(Vext, OUTPUT);
  digitalWrite(Vext, LOW);
}

void VextOFF(void) {
  pinMode(Vext, OUTPUT);
  digitalWrite(Vext, HIGH);
}

void loop() {

  dp.clear();

  dp.setTextAlignment(TEXT_ALIGN_LEFT);
  dp.setFont(ArialMT_Plain_16);
  dp.drawString(0, 0, "START");

  dp.setTextAlignment(TEXT_ALIGN_CENTER);
  dp.setFont(ArialMT_Plain_16);
  dp.drawString(64, 25, String(timeTrigger));

  dp.display();

  if (buttonTriggered == true) {

    buttonTriggered = false;
    lora.sendData(timeTrigger);
  }

  lora.proccess();

  delay(10);
}
