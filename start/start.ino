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
  BTN_WAITING_RELEASE,
  BTN_DEBOUNCE_RELEASE
};
ButtonState buttonState = BTN_IDLE;
unsigned long buttonPressStartTime = 0;
unsigned long buttonReleaseTime = 0;

char timeTriggerStr[32] = "-";
char syncMsg[32];
char uptimeStr[32];
char batteryStr[8];
char rxMsg[64];
int16_t rxLen;
int16_t rxRssi;
int8_t rxSnr;
uint64_t rxTs = 0;
int syncSamplesCompleted = 0;
bool syncDone = false;

// Must match TIMESYNC_SAMPLES in finish/timesync.h
static const int SYNC_SAMPLES_REQUIRED = 15;

unsigned long lastSyncTime = (unsigned long)(0UL - SYNC_INTERVAL_MS);
unsigned long lastDisplayUpdate = 0;

#define BURST_COOLDOWN_MS 2000
unsigned long lastBurstTime = 0;
bool burstCooldownActive = false;

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

        if (duration >= 30 && !burstCooldownActive) {
          // Read 64-bit volatile in a critical section to avoid torn reads
          noInterrupts();
          uint64_t capturedMicros = buttonPressMicros;
          interrupts();

          char startMsg[40];
          snprintf(startMsg, sizeof(startMsg), "time:%llu", capturedMicros);
          snprintf(timeTriggerStr, sizeof(timeTriggerStr), "%llu", capturedMicros);

          lora.sendData(startMsg, true);
          Serial.printf("[BTN] Start-Trigger (Burst): %s\n", startMsg);

          lastBurstTime = currentMillis;
          burstCooldownActive = true;
        }

        buttonState = BTN_IDLE;

      } else if (currentMillis - buttonPressStartTime >= 5000) {
        buttonState = BTN_WAITING_RELEASE;
      }

      break;

    case BTN_WAITING_RELEASE:

      if (digitalRead(BUTTON_PIN) == HIGH) {
        buttonReleaseTime = currentMillis;
        buttonState = BTN_DEBOUNCE_RELEASE;
      }
      break;

    case BTN_DEBOUNCE_RELEASE:
      // Wait 50 ms without blocking, then confirm stable release
      if (currentMillis - buttonReleaseTime >= 50) {
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

  if (burstCooldownActive && (currentMillis - lastBurstTime >= BURST_COOLDOWN_MS)) {
    burstCooldownActive = false;
    Serial.println("[BTN] Burst cooldown ready, Ready for new event.");
  }

  handleButtonStateMachine(currentMillis);

  if (currentMillis - lastDisplayUpdate >= DISPLAY_UPDATE_INTERVAL) {
    lastDisplayUpdate = currentMillis;

    snprintf(uptimeStr, sizeof(uptimeStr), "Up: %lus", currentMillis / 1000);
    snprintf(batteryStr, sizeof(batteryStr), "%d%%", battery.getPercentage());

    dp.clear();

    dp.setTextAlignment(TEXT_ALIGN_LEFT);
    dp.setFont(ArialMT_Plain_16);
    if (syncDone) {
      dp.drawString(0, 0, "START S:OK");
    } else if (syncSamplesCompleted > 0) {
      char hdr[22];
      snprintf(hdr, sizeof(hdr), "START S:%d/%d", syncSamplesCompleted, SYNC_SAMPLES_REQUIRED);
      dp.drawString(0, 0, hdr);
    } else {
      dp.drawString(0, 0, "START");
    }

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

    if (!burstCooldownActive) {
      snprintf(syncMsg, sizeof(syncMsg), "sync:%llu", (uint64_t)esp_timer_get_time());
      lora.sendData(syncMsg);
      Serial.printf("[SYNC] sync sent: %s\n", syncMsg);
    } else {
      Serial.println("[SYNC] sync skipped (burst cooldown active)");
    }
  }

  if (lora.tryGetPacket(rxMsg, sizeof(rxMsg), rxLen, rxRssi, rxSnr, rxTs)) {

    if (strncmp(rxMsg, "syncack:", 8) == 0) {

      uint64_t t1 = strtoull(&rxMsg[8], nullptr, 10);
      uint64_t t4 = rxTs;
      int64_t halfRtt = (int64_t)((t4 - t1) / 2);

      char finMsg[64];
      snprintf(finMsg, sizeof(finMsg), "syncfin:%llu:%lld", t1, halfRtt);
      lora.sendData(finMsg);

      if (syncDone) {

        Serial.println("[SYNC] Re-sync detected (finish rebooted) — resetting sync counter");
        syncDone = false;
        syncSamplesCompleted = 0;
      }
      syncSamplesCompleted++;
      if (syncSamplesCompleted >= SYNC_SAMPLES_REQUIRED) {
        syncDone = true;
      }

      Serial.printf("[SYNC] syncfin sent: t1=%llu  t4=%llu  half_rtt=%lld us  (%d/%d samples)\n", t1, t4, halfRtt, syncSamplesCompleted, SYNC_SAMPLES_REQUIRED);
    }
  }

  vTaskDelay(1);
}