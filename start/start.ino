#include <Wire.h>
#include "display.h"
#include "lora.h"

#define BUTTON_PIN 0
#define SYNC_INTERVAL_MS 5000

Display dp;
Lora lora;

volatile bool buttonTriggered = false;
volatile unsigned long lastButtonPress = 0;
volatile unsigned long buttonPressMicros = 0;
const unsigned long DISPLAY_UPDATE_INTERVAL = 200;

char timeTriggerStr[32] = "-";
char syncMsg[32];
char uptimeStr[32];

unsigned long lastSyncTime = -SYNC_INTERVAL_MS;
unsigned long lastDisplayUpdate = 0;

void VextON(void) {
  pinMode(Vext, OUTPUT);
  digitalWrite(Vext, LOW);
}

void VextOFF(void) {
  pinMode(Vext, OUTPUT);
  digitalWrite(Vext, HIGH);
}

void IRAM_ATTR handleButtonPress() {
  buttonTriggered = true;
}

void showChipId() {
  uint64_t chipId = ESP.getEfuseMac();
  Serial.printf("ESP32 Chip model = %s Rev %d\n", ESP.getChipModel(), ESP.getChipRevision());
  Serial.printf("This chip has %d cores\n", ESP.getChipCores());
  Serial.printf("ESP32ChipID=%04X%08X\r\n", (uint16_t)(chipId >> 32), (uint32_t)chipId);
}

void setup() {
  Serial.begin(115200);
  Serial.println();

  showChipId();

  lora.preInit();
  VextON();
  delay(100);

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  attachInterrupt(BUTTON_PIN, handleButtonPress, FALLING);

  dp.init();
  lora.init();
  lora.startTask();

  dp.setFont(ArialMT_Plain_10);
}

void loop() {
  unsigned long currentMillis = millis();

  if (buttonTriggered) {
    if (currentMillis - lastButtonPress > 200) {
      lastButtonPress = currentMillis;
      snprintf(timeTriggerStr, sizeof(timeTriggerStr), "%lu", micros());
      lora.sendData(timeTriggerStr);
    }
    buttonTriggered = false;
  }

  if (currentMillis - lastDisplayUpdate >= DISPLAY_UPDATE_INTERVAL) {
    lastDisplayUpdate = currentMillis;

    snprintf(uptimeStr, sizeof(uptimeStr), "UpTime: %lus", currentMillis / 1000);

    dp.clear();

    dp.setTextAlignment(TEXT_ALIGN_LEFT);
    dp.setFont(ArialMT_Plain_16);
    dp.drawString(0, 0, "START");

    dp.setTextAlignment(TEXT_ALIGN_CENTER);
    dp.setFont(ArialMT_Plain_10);
    dp.drawString(64, 25, timeTriggerStr);

    dp.setTextAlignment(TEXT_ALIGN_LEFT);
    dp.setFont(ArialMT_Plain_10);
    dp.drawString(0, 54, uptimeStr);

    dp.display();
  }

  if (currentMillis - lastSyncTime >= SYNC_INTERVAL_MS) {
    lastSyncTime = currentMillis;

    snprintf(syncMsg, sizeof(syncMsg), "sync:%llu", (uint64_t)micros());
    lora.sendData(syncMsg);
    Serial.printf("send sync: %s\n", syncMsg);
  }

  vTaskDelay(1);
}