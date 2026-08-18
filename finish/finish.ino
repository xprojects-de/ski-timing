#include <Wire.h>
#include "stopwatch.h"
#include "display.h"
#include "lora.h"
#include "accesspoint.h"
#include "customWebserver.h"
#include "customFilesystem.h"
#include "timesync.h"

#define BUTTON_PIN 0
#define SYNC_LED_PIN 35

StopWatch stopWatch;
Display dp;
Lora lora;
AccessPoint accessPoint;
CustomWebserver webserver;
CFs fs_instance;
Timesync timeSync;

volatile bool buttonTriggered = false;
unsigned long lastDisplayUpdate = 0;
const unsigned long DISPLAY_UPDATE_INTERVAL = 200;
unsigned long lastButtonPress = 0;

char msg[64];
int16_t len, rssi;
int8_t snr;

void IRAM_ATTR handleButtonPress() {
  buttonTriggered = true;
}

void VextON(void) {
  pinMode(Vext, OUTPUT);
  digitalWrite(Vext, LOW);
}

void VextOFF(void) {
  pinMode(Vext, OUTPUT);
  digitalWrite(Vext, HIGH);
}

void showChipId() {
  uint64_t chipId = ESP.getEfuseMac();
  Serial.printf("ESP32 Chip model = %s Rev %d\n", ESP.getChipModel(), ESP.getChipRevision());
  Serial.printf("This chip has %d cores\n", ESP.getChipCores());
  Serial.printf("ESP32ChipID=%04X%08X\r\n", (uint16_t)(chipId >> 32), (uint32_t)chipId);
}

void setup() {
  Serial.begin(115200);
  Serial.println("\n\n--- SKI TIMING FINISH ---");

  VextON();
  delay(50);

  showChipId();

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  attachInterrupt(BUTTON_PIN, handleButtonPress, FALLING);

  pinMode(SYNC_LED_PIN, OUTPUT);
  digitalWrite(SYNC_LED_PIN, LOW);

  lora.preInit();
  dp.init();
  lora.init();
  lora.startTask();

  accessPoint.init();
  fs_instance.init();
  webserver.init(&fs_instance, &stopWatch);

  dp.setFont(ArialMT_Plain_10);
}

void loop() {
  unsigned long currentMillis = millis();

  if (currentMillis - lastDisplayUpdate >= DISPLAY_UPDATE_INTERVAL) {
    lastDisplayUpdate = currentMillis;

    dp.clear();

    dp.setTextAlignment(TEXT_ALIGN_LEFT);
    dp.setFont(ArialMT_Plain_16);
    dp.drawString(0, 0, "FINISH");

    dp.setTextAlignment(TEXT_ALIGN_CENTER);
    dp.setFont(ArialMT_Plain_10);
    dp.drawString(64, 25, stopWatch.draw());

    dp.setTextAlignment(TEXT_ALIGN_LEFT);
    dp.setFont(ArialMT_Plain_10);

    if (timeSync.isInit()) {
      dp.drawString(0, 54, "synced - UpTime: " + String(currentMillis / 1000) + "s");
      digitalWrite(SYNC_LED_PIN, HIGH);
    } else {
      dp.drawString(0, 54, "UpTime: " + String(currentMillis / 1000) + "s");
      digitalWrite(SYNC_LED_PIN, LOW);
    }

    dp.display();
  }

  if (buttonTriggered) {
    if (currentMillis - lastButtonPress > 200) {
      lastButtonPress = currentMillis;
      stopWatch.triggerFinish(&fs_instance);
    }
    buttonTriggered = false;
  }

  if (lora.available()) {
    lora.getPacket(msg, len, rssi, snr);

    Serial.printf("Receive message: %s | RSSI: %d dBm | SNR: %d dB\n", msg, rssi, snr);

    if (strncmp(msg, "sync:", 5) == 0) {
      timeSync.syncTime(atoll(&msg[5]), &stopWatch);
    } else {
      if (timeSync.isInit()) {
        stopWatch.triggerStart(atoll(msg));
      }
    }
  }

  vTaskDelay(1);
}