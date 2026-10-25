#include <Wire.h>
#include "display.h"
#include "lora.h"
#include "batteryCheck.h"
#include <esp_timer.h>

#define BUTTON_PIN 0
#define TRIGGER_PIN 6  // external start switch, active LOW like the button (wiring: docs/hardware.md)
#define TRIGGER_CONFIRM_US 10000  // the gate stays open, so a real start is still LOW after this; interference is not
#define SYNC_INTERVAL_MS 5000
#define HEAP_LOG_INTERVAL_MS 60000  // diagnostic: watch for heap fragmentation over long runtimes

// Button hold-time thresholds (ms)
#define BTN_MIN_VALID_PRESS_MS 30     // ignore glitches/bounce shorter than this
#define BTN_SHUTDOWN_ARM_HOLD_MS 7500 // still held beyond this arms deep-sleep on release
#define BTN_RELEASE_DEBOUNCE_MS 50    // stable-release debounce before shutdown

Display dp;
Lora lora;
BatteryCheck battery;

volatile bool buttonTriggered = false;
volatile uint64_t buttonPressMicros = 0;

// Set by the ISR on the first falling edge; cleared once the gate is closed again (input HIGH).
volatile bool triggerInputFired = false;
volatile uint64_t triggerInputMicros = 0;
bool triggerInputHandled = false;

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

unsigned long lastSyncTime = (unsigned long)(0UL - SYNC_INTERVAL_MS);
unsigned long lastDisplayUpdate = 0;
unsigned long lastHeapLogTime = 0;

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

void IRAM_ATTR handleTriggerInputFalling() {
  if (!triggerInputFired) {
    triggerInputMicros = esp_timer_get_time();
    triggerInputFired = true;
  }
}

void showChipId() {
  uint64_t chipId = ESP.getEfuseMac();
  Serial.printf("ESP32 Chip model = %s Rev %d\n", ESP.getChipModel(), ESP.getChipRevision());
  Serial.printf("This chip has %d cores\n", ESP.getChipCores());
  Serial.printf("ESP32ChipID=%04X%08X\r\n", (uint16_t)(chipId >> 32), (uint32_t)chipId);
}

void sendStartTrigger(uint64_t triggerMicros, unsigned long currentMillis, const char* logTag) {
  char startMsg[40];
  snprintf(startMsg, sizeof(startMsg), "time:%llu", triggerMicros);
  snprintf(timeTriggerStr, sizeof(timeTriggerStr), "%llu", triggerMicros);

  lora.sendData(startMsg, true);
  Serial.printf("[%s] Start-Trigger (Burst): %s\n", logTag, startMsg);

  lastBurstTime = currentMillis;
  burstCooldownActive = true;
}

// The external input only triggers the start - no hold-time functions, the gate staying open must not put the
// unit to sleep. The edge time is sent once the input is still LOW TRIGGER_CONFIRM_US later; the ISR is re-armed
// only when the gate is closed again, so bounce cannot overwrite the time of the first contact.
void handleTriggerInput(unsigned long currentMillis) {
  if (!triggerInputFired) return;

  noInterrupts();
  uint64_t edgeMicros = triggerInputMicros;
  interrupts();

  if (esp_timer_get_time() - edgeMicros < TRIGGER_CONFIRM_US) return;

  // HIGH again: the gate was closed for the next racer - or, if nothing was sent, it was interference
  if (digitalRead(TRIGGER_PIN) == HIGH) {
    triggerInputHandled = false;
    triggerInputFired = false;  // re-arms the ISR
    return;
  }

  if (triggerInputHandled) return;
  triggerInputHandled = true;

  if (burstCooldownActive) {
    Serial.println("[TRIGGER] Start ignored: burst cooldown active");
    return;
  }
  sendStartTrigger(edgeMicros, currentMillis, "TRIGGER");
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

        if (duration >= BTN_MIN_VALID_PRESS_MS && !burstCooldownActive) {
          // Read 64-bit volatile in a critical section to avoid torn reads
          noInterrupts();
          uint64_t capturedMicros = buttonPressMicros;
          interrupts();

          sendStartTrigger(capturedMicros, currentMillis, "BTN");
        }

        buttonState = BTN_IDLE;

      } else if (currentMillis - buttonPressStartTime >= BTN_SHUTDOWN_ARM_HOLD_MS) {
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
      // Wait BTN_RELEASE_DEBOUNCE_MS without blocking, then confirm stable release
      if (currentMillis - buttonReleaseTime >= BTN_RELEASE_DEBOUNCE_MS) {
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
  detachInterrupt(TRIGGER_PIN);

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

  pinMode(TRIGGER_PIN, INPUT_PULLUP);  // backup for the external pull-up, keeps an unplugged input HIGH
  attachInterrupt(TRIGGER_PIN, handleTriggerInputFalling, FALLING);

  dp.init();
  battery.init();
  battery.startTask();
  lora.init();
  lora.startTask();

  dp.setFont(ArialMT_Plain_10);
}

void loop() {

  unsigned long currentMillis = millis();

  if (burstCooldownActive && (currentMillis - lastBurstTime >= BURST_COOLDOWN_MS)) {
    burstCooldownActive = false;
    Serial.println("[BTN] Burst cooldown ready, Ready for new event.");
  }

  handleButtonStateMachine(currentMillis);
  handleTriggerInput(currentMillis);

  if (currentMillis - lastHeapLogTime >= HEAP_LOG_INTERVAL_MS) {
    lastHeapLogTime = currentMillis;
    Serial.printf("[HEAP] free=%u min=%u maxAlloc=%u\n",
                  ESP.getFreeHeap(), ESP.getMinFreeHeap(), ESP.getMaxAllocHeap());
  }

  if (currentMillis - lastDisplayUpdate >= DISPLAY_UPDATE_INTERVAL) {
    lastDisplayUpdate = currentMillis;

    snprintf(uptimeStr, sizeof(uptimeStr), "Up: %lus", currentMillis / 1000);
    snprintf(batteryStr, sizeof(batteryStr), "%d%%", battery.getPercentage());

    dp.clear();

    dp.setTextAlignment(TEXT_ALIGN_LEFT);
    dp.setFont(ArialMT_Plain_16);
    if (syncSamplesCompleted > 0) {
      char hdr[20];
      snprintf(hdr, sizeof(hdr), "START (%d)", syncSamplesCompleted);
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
      // Same length as the finish's "syncack:<t1>": halfRtt assumes both legs take the same air time, and at
      // SF10 a few bytes more can cost one more LoRa block (~65 ms) on one way.
      snprintf(syncMsg, sizeof(syncMsg), "syncreq:%llu", (uint64_t)esp_timer_get_time());
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

      syncSamplesCompleted++;
      Serial.printf("[SYNC] syncfin sent: t1=%llu  t4=%llu  half_rtt=%lld us  (sample %d)\n", t1, t4, halfRtt, syncSamplesCompleted);
    }
  }

  vTaskDelay(1);
}