#include <Wire.h>
#include "stopwatch.h"
#include "display.h"
#include "lora.h"
#include "accesspoint.h"
#include "customWebserver.h"
#include "customFilesystem.h"
#include "timesync.h"
#include "batteryCheck.h"
#include <WiFi.h>
#include <esp_timer.h>

#define BUTTON_PIN 0
#define SYNC_LED_PIN 35

StopWatch stopWatch;
Display dp;
Lora lora;
AccessPoint accessPoint;
CustomWebserver webserver;
CFs fs_instance;
Timesync timeSync;
BatteryCheck battery;

volatile bool buttonTriggered = false;
volatile uint64_t buttonPressMicros = 0;
unsigned long lastDisplayUpdate = 0;
const unsigned long DISPLAY_UPDATE_INTERVAL = 250;

enum ButtonState {
  BTN_IDLE,
  BTN_PRESSED,
  BTN_WAITING_RELEASE
};
ButtonState buttonState = BTN_IDLE;
unsigned long buttonPressStartTime = 0;
bool shutdownWarningShown = false;

char msg[64];
int16_t len, rssi;
int8_t snr;

char batteryStr[8];
char uptimeStr[32];
char timeStr[32];

void IRAM_ATTR handleButtonPress() {
  if (!buttonTriggered) {
    buttonPressMicros = esp_timer_get_time();
    buttonTriggered = true;
  }
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

void handleButtonStateMachine(unsigned long currentMillis) {

  switch (buttonState) {

    case BTN_IDLE:

      if (buttonTriggered) {
        buttonTriggered = false;
        buttonState = BTN_PRESSED;
        buttonPressStartTime = currentMillis;
        shutdownWarningShown = false;
      }

      break;

    case BTN_PRESSED:

      if (digitalRead(BUTTON_PIN) == HIGH) {

        unsigned long duration = currentMillis - buttonPressStartTime;

        if (duration >= 2500) {

          stopWatch.setContinuousMode(!stopWatch.isContinuousMode());
          stopWatch.reset();
          Serial.printf("[BTN] Mode changed to: %s\n", stopWatch.isContinuousMode() ? "Continuous" : "Normal");
        } else if (duration >= 200) {
          stopWatch.triggerFinishWithTimestamp(&fs_instance, buttonPressMicros);
          Serial.println("[BTN] Finish triggered");
        }

        buttonState = BTN_IDLE;

      } else if (currentMillis - buttonPressStartTime >= 5000) {

        if (!shutdownWarningShown) {
          dp.clear();
          dp.setFont(ArialMT_Plain_16);
          dp.setTextAlignment(TEXT_ALIGN_CENTER);
          dp.drawString(64, 20, "RELEASE TO");
          dp.drawString(64, 40, "SHUTDOWN");
          dp.display();
          shutdownWarningShown = true;
          Serial.println("[BTN] Shutdown warning shown");
        }

        buttonState = BTN_WAITING_RELEASE;
      }

      break;

    case BTN_WAITING_RELEASE:
      if (digitalRead(BUTTON_PIN) == HIGH) {

        delay(50);

        if (digitalRead(BUTTON_PIN) == HIGH) {
          powerDownAndSleep();
        } else {
          buttonState = BTN_WAITING_RELEASE;
        }
      }

      break;
  }
}

void powerDownAndSleep() {

  Serial.println("\n[POWER] Shutdown startet...");

  dp.clear();
  dp.setFont(ArialMT_Plain_16);
  dp.setTextAlignment(TEXT_ALIGN_CENTER);
  dp.drawString(64, 20, "SHUTDOWN...");
  dp.display();
  delay(1000);

  detachInterrupt(BUTTON_PIN);

  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);

  digitalWrite(SYNC_LED_PIN, LOW);
  dp.clear();
  dp.display();

  VextOFF();

  esp_sleep_enable_ext0_wakeup((gpio_num_t)BUTTON_PIN, 0);

  Serial.println("[POWER] Go to sleep.");
  Serial.flush();

  esp_deep_sleep_start();
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

  battery.init();

  dp.setFont(ArialMT_Plain_10);
}

void loop() {
  unsigned long currentMillis = millis();

  battery.update(currentMillis);

  if (currentMillis - lastDisplayUpdate >= DISPLAY_UPDATE_INTERVAL) {
    lastDisplayUpdate = currentMillis;

    snprintf(batteryStr, sizeof(batteryStr), "%d%%", battery.getPercentage());
    snprintf(timeStr, sizeof(timeStr), "%s", stopWatch.draw().c_str());

    if (timeSync.isInit()) {
      snprintf(uptimeStr, sizeof(uptimeStr), "sync - Up: %lus", currentMillis / 1000);
    } else {
      snprintf(uptimeStr, sizeof(uptimeStr), "Up: %lus", currentMillis / 1000);
    }

    dp.clear();

    dp.setTextAlignment(TEXT_ALIGN_LEFT);
    dp.setFont(ArialMT_Plain_16);

    if (stopWatch.isContinuousMode()) {
      dp.drawString(0, 0, "FINISH (C)");
    } else {
      dp.drawString(0, 0, "FINISH (N)");
    }

    dp.setTextAlignment(TEXT_ALIGN_RIGHT);
    dp.setFont(ArialMT_Plain_10);
    dp.drawString(128, 0, batteryStr);

    dp.setTextAlignment(TEXT_ALIGN_CENTER);
    dp.setFont(ArialMT_Plain_10);
    dp.drawString(64, 25, timeStr);

    dp.setTextAlignment(TEXT_ALIGN_LEFT);
    dp.setFont(ArialMT_Plain_10);
    dp.drawString(0, 54, uptimeStr);

    digitalWrite(SYNC_LED_PIN, timeSync.isInit() ? HIGH : LOW);

    dp.display();
  }

  handleButtonStateMachine(currentMillis);

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