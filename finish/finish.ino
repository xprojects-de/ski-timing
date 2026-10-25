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
#define TRIGGER_PIN 6  // external light barrier via optocoupler, active LOW like the button (wiring: docs/hardware.md)
#define TRIGGER_MIN_PULSE_US 1000  // shorter LOW pulses are interference on the cable (a leg in the beam: ~3 ms)
#define SYNC_LED_PIN 35
#define HEAP_LOG_INTERVAL_MS 60000  // diagnostic: watch for heap fragmentation over long runtimes

// Button hold-time thresholds (ms)
#define BTN_MIN_VALID_PRESS_MS 30     // ignore glitches/bounce shorter than this
#define BTN_FALL_DETECT_HOLD_MS 1000  // hold to discard oldest queued start time
#define BTN_MODE_TOGGLE_HOLD_MS 4000  // hold to toggle Normal/Continuous mode
#define BTN_SHUTDOWN_ARM_HOLD_MS 7500 // still held beyond this arms deep-sleep on release
#define BTN_RELEASE_DEBOUNCE_MS 50    // stable-release debounce before shutdown

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

// Trigger input: the ISR reports a pulse at its rising edge, with the time of its falling edge, if it lasted
// TRIGGER_MIN_PULSE_US. Shorter pulses - interference, bounce while releasing - are ignored.
volatile uint64_t triggerFallMicros = 0;
volatile bool triggerPending = false;
volatile uint64_t triggerPulseMicros = 0;

unsigned long lastDisplayUpdate = 0;
unsigned long lastHeapLogTime = 0;
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

char msg[64];
int16_t len, rssi;
int8_t snr;
uint64_t rxTs = 0;

char batteryStr[8];
char uptimeStr[32];
char timeStr[32];

void IRAM_ATTR handleButtonPress() {
  if (!buttonTriggered) {
    buttonPressMicros = esp_timer_get_time();
    buttonTriggered = true;
  }
}

void IRAM_ATTR handleTriggerInputChange() {
  uint64_t now = esp_timer_get_time();

  if (digitalRead(TRIGGER_PIN) == LOW) {
    triggerFallMicros = now;
    return;
  }

  if (!triggerPending && now - triggerFallMicros >= TRIGGER_MIN_PULSE_US) {
    triggerPulseMicros = triggerFallMicros;
    triggerPending = true;
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

// The external input only triggers the finish - no hold-time functions: a beam blocked for seconds (someone
// standing in it, snow on the reflector) must not discard a start, switch the mode or put the unit to sleep.
// A second break by legs or poles is dropped by the stopwatch debounce, shared with the button.
void handleTriggerInput() {
  noInterrupts();
  bool pending = triggerPending;
  uint64_t pulseMicros = triggerPulseMicros;
  triggerPending = false;
  interrupts();

  if (!pending) return;

  stopWatch.triggerFinishWithTimestamp(&fs_instance, pulseMicros);
  Serial.println("[TRIGGER] Finish triggered");
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

        if (duration >= BTN_MODE_TOGGLE_HOLD_MS) {

          stopWatch.setContinuousMode(!stopWatch.isContinuousMode());
          stopWatch.reset();
          fs_instance.clear();
          Serial.printf("[BTN] Mode changed to: %s (times cleared)\n", stopWatch.isContinuousMode() ? "Continuous" : "Normal");
        } else if (duration >= BTN_FALL_DETECT_HOLD_MS) {

          if (!stopWatch.isContinuousMode()) {
            if (stopWatch.discardOldestStart()) {
              Serial.println("[BTN] Fall detected: Start time removed.");
            } else {
              Serial.println("[BTN] Fall detected: no startTime in queue!");
            }
          } else {
            Serial.println("[BTN] Fall detected: not possible in continuousMode.");
          }
        } else if (duration >= BTN_MIN_VALID_PRESS_MS) {
          // Read 64-bit volatile in a critical section to avoid torn reads
          noInterrupts();
          uint64_t capturedMicros = buttonPressMicros;
          interrupts();
          stopWatch.triggerFinishWithTimestamp(&fs_instance, capturedMicros);
          Serial.println("[BTN] Finish triggered");
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

  pinMode(TRIGGER_PIN, INPUT_PULLUP);  // backup for the external pull-up, keeps an unplugged input HIGH
  attachInterrupt(TRIGGER_PIN, handleTriggerInputChange, CHANGE);

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
  battery.startTask();

  dp.setFont(ArialMT_Plain_10);
}

void loop() {
  unsigned long currentMillis = millis();

  if (currentMillis - lastHeapLogTime >= HEAP_LOG_INTERVAL_MS) {
    lastHeapLogTime = currentMillis;
    Serial.printf("[HEAP] free=%u min=%u maxAlloc=%u\n",
                  ESP.getFreeHeap(), ESP.getMinFreeHeap(), ESP.getMaxAllocHeap());
  }

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
      char headerStr[16];
      snprintf(headerStr, sizeof(headerStr), "FINISH C:%d", fs_instance.getCount());
      dp.drawString(0, 0, headerStr);
    } else {
      char headerStr[16];
      int queueSize = stopWatch.getQueueSize();
      snprintf(headerStr, sizeof(headerStr), "FINISH (%d)", queueSize);
      dp.drawString(0, 0, headerStr);
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
  handleTriggerInput();

  if (lora.tryGetPacket(msg, sizeof(msg), len, rssi, snr, rxTs)) {

    Serial.printf("Receive message: %s | RSSI: %d dBm | SNR: %d dB\n", msg, rssi, snr);

    if (strncmp(msg, "syncreq:", 8) == 0) {

      uint64_t t1 = strtoull(&msg[8], nullptr, 10);
      if (timeSync.onSyncMsg(t1, rxTs)) {

        char ack[32];
        snprintf(ack, sizeof(ack), "syncack:%llu", t1);
        lora.sendData(ack);
        Serial.printf("[TIMESYNC] syncack sent for t1=%llu\n", t1);
      }

    } else if (strncmp(msg, "syncfin:", 8) == 0) {

      uint64_t t1 = 0;
      int64_t halfRtt = 0;
      sscanf(&msg[8], "%llu:%lld", &t1, &halfRtt);
      timeSync.onSyncFinMsg(t1, halfRtt, &stopWatch);

    } else if (strncmp(msg, "time:", 5) == 0) {

      if (timeSync.isInit()) {
        uint64_t startTs = strtoull(&msg[5], nullptr, 10);
        stopWatch.triggerStart(startTs);
      }
    }
  }

  vTaskDelay(1);
}