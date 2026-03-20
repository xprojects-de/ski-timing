// For a connection via I2C using the Arduino Wire include:
#include <Wire.h>
#include "stopwatch.h"
#include "display.h"
#include "lora.h"
#include "accesspoint.h"
#include "webserver.h"
#include "fs.h"
#include "timesync.h"

#define BUTTON_PIN 0
#define SYNC_LED_PIN 35

StopWatch stopWatch;
Display dp;
Lora lora;
AccessPoint accessPoint;
Webserver webserver;
Fs fs;
Timesync timeSync;

volatile bool buttonTriggered = false;

char msg[64];
int16_t len, rssi;
int8_t snr;

void IRAM_ATTR handleButtonPress() {
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
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  attachInterrupt(BUTTON_PIN, handleButtonPress, FALLING);

  pinMode(SYNC_LED_PIN, OUTPUT);
  digitalWrite(SYNC_LED_PIN, LOW);

  delay(100);

  dp.init();
  lora.init();
  accessPoint.init();
  fs.init();
  webserver.init(&fs);

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
  dp.drawString(64, 25, stopWatch.draw());

  dp.setTextAlignment(TEXT_ALIGN_LEFT);
  dp.setFont(ArialMT_Plain_10);

  if (timeSync.isInit()) {
    dp.drawString(0, 54, "synced - UpTime: " + String(millis() / 1000) + "s");
    digitalWrite(SYNC_LED_PIN, HIGH);
  } else {
    dp.drawString(0, 54, "UpTime: " + String(millis() / 1000) + "s");
    digitalWrite(SYNC_LED_PIN, LOW);
  }

  dp.display();

  if (buttonTriggered == true) {
    buttonTriggered = false;
    stopWatch.triggerFinish(&fs);
  }

  lora.proccess();

  if (lora.available() == true) {

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

  delay(5);
}
