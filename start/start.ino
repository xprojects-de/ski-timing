#include <Wire.h>
#include "display.h"
#include "lora.h"
#include "batteryCheck.h"

#define BUTTON_PIN 0
#define SYNC_INTERVAL_MS 5000

Display dp;
Lora lora;
BatteryCheck battery;

volatile bool buttonTriggered = false;
volatile uint64_t buttonPressMicros = 0;
volatile unsigned long lastButtonPress = 0;
const unsigned long DISPLAY_UPDATE_INTERVAL = 200;

enum ButtonState {
  BTN_IDLE,
  BTN_PRESSED,
  BTN_WAITING_RELEASE
};
ButtonState buttonState = BTN_IDLE;
unsigned long buttonPressStartTime = 0;
bool shutdownWarningShown = false;

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
  buttonPressMicros = esp_timer_get_time();
  buttonTriggered = true;
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

      if (currentMillis - buttonPressStartTime >= 5000) {

        if (!shutdownWarningShown) {
          dp.clear();
          dp.setFont(ArialMT_Plain_16);
          dp.setTextAlignment(TEXT_ALIGN_CENTER);
          dp.drawString(64, 20, "RELEASE TO");
          dp.drawString(64, 40, "SHUTDOWN");
          dp.display();
          shutdownWarningShown = true;
        }
        buttonState = BTN_WAITING_RELEASE;
      } else if (digitalRead(BUTTON_PIN) == HIGH) {

        if (currentMillis - lastButtonPress > 200) {
          lastButtonPress = currentMillis;
          snprintf(timeTriggerStr, sizeof(timeTriggerStr), "%llu", buttonPressMicros);
          lora.sendData(timeTriggerStr);
        }

        buttonState = BTN_IDLE;
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

    dp.clear();

    dp.setTextAlignment(TEXT_ALIGN_LEFT);
    dp.setFont(ArialMT_Plain_16);
    dp.drawString(0, 0, "START");

    dp.setTextAlignment(TEXT_ALIGN_RIGHT);
    dp.setFont(ArialMT_Plain_10);
    dp.drawString(128, 0, String(battery.getPercentage()) + "%");

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
    Serial.printf("send sync: %s\n", syncMsg);
  }

  vTaskDelay(1);
}