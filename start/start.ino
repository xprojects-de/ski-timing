#include <Wire.h>
#include "display.h"
#include "lora.h"
#include "batteryCheck.h"
#include <esp_timer.h>

#define BUTTON_PIN 0
#define SYNC_INTERVAL_MS 5000

Display dp;
Lora lora;
BatteryCheck battery;

volatile bool buttonTriggered = false;
volatile uint64_t buttonPressMicros = 0;
const unsigned long DISPLAY_UPDATE_INTERVAL = 250;

enum ButtonState {
  BTN_IDLE,
  BTN_PRESSED,
  BTN_WAITING_RELEASE
};
ButtonState buttonState = BTN_IDLE;
unsigned long buttonPressStartTime = 0;

char timeTriggerStr[32] = "-";
char syncMsg[32];
char uptimeStr[32];
char batteryStr[8];
char rxMsg[64];
int16_t rxLen;
int16_t rxRssi;
int8_t  rxSnr;

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
  if (!buttonTriggered) {
    buttonPressMicros = esp_timer_get_time();
    buttonTriggered = true;
  }
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
      }
      break;

    case BTN_PRESSED:

      if (digitalRead(BUTTON_PIN) == HIGH) {

        unsigned long duration = currentMillis - buttonPressStartTime;

        if (duration >= 30) {
          snprintf(timeTriggerStr, sizeof(timeTriggerStr), "%llu", buttonPressMicros);
          lora.sendData(timeTriggerStr);
          Serial.printf("[BTN] Start trigger sent: %s\n", timeTriggerStr);
        }

        buttonState = BTN_IDLE;

      } else if (currentMillis - buttonPressStartTime >= 5000) {
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

  dp.clear();
  dp.display();

  VextOFF();

  esp_sleep_enable_ext0_wakeup((gpio_num_t)BUTTON_PIN, 0);

  Serial.println("[POWER] Gehe in Deep Sleep.");
  Serial.flush();

  esp_deep_sleep_start();
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
  battery.init();
  lora.init();
  lora.startTask();

  dp.setFont(ArialMT_Plain_10);
}

void loop() {

  unsigned long currentMillis = millis();

  battery.update(currentMillis);

  handleButtonStateMachine(currentMillis);

  if (currentMillis - lastDisplayUpdate >= DISPLAY_UPDATE_INTERVAL) {
    lastDisplayUpdate = currentMillis;

    snprintf(uptimeStr, sizeof(uptimeStr), "UpTime: %lus", currentMillis / 1000);
    snprintf(batteryStr, sizeof(batteryStr), "%d%%", battery.getPercentage());

    dp.clear();

    dp.setTextAlignment(TEXT_ALIGN_LEFT);
    dp.setFont(ArialMT_Plain_16);
    dp.drawString(0, 0, "START");

    dp.setTextAlignment(TEXT_ALIGN_RIGHT);
    dp.setFont(ArialMT_Plain_10);
    dp.drawString(128, 0, batteryStr);

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

    snprintf(syncMsg, sizeof(syncMsg), "sync:%llu", (uint64_t)esp_timer_get_time());
    lora.sendData(syncMsg);
    Serial.printf("[SYNC] sync sent: %s\n", syncMsg);
  }

  if (lora.available()) {
    lora.getPacket(rxMsg, rxLen, rxRssi, rxSnr);

    if (strncmp(rxMsg, "syncack:", 8) == 0) {

      uint64_t t1      = strtoull(&rxMsg[8], nullptr, 10);
      uint64_t t4      = (uint64_t)esp_timer_get_time();
      int64_t halfRtt  = (int64_t)((t4 - t1) / 2);

      char finMsg[64];
      snprintf(finMsg, sizeof(finMsg), "syncfin:%llu:%lld", t1, halfRtt);
      lora.sendData(finMsg);
      Serial.printf("[SYNC] syncfin sent: t1=%llu  half_rtt=%lld us\n", t1, halfRtt);
    }
  }

  vTaskDelay(1);
}