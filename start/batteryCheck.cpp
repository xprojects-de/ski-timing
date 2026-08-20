#include "batteryCheck.h"
#include <esp_adc_cal.h>

BatteryCheck::BatteryCheck(uint8_t adc_pin, uint8_t ctrl_pin, unsigned long interval_ms)
  : adcPin(adc_pin), ctrlPin(ctrl_pin), cachedPercentage(0), lastCheckTime(0), checkInterval(interval_ms) {}

void BatteryCheck::init() {

  pinMode(ctrlPin, OUTPUT);
  digitalWrite(ctrlPin, HIGH);

  delay(50);

  forceUpdate();

  lastCheckTime = millis();
}

uint16_t BatteryCheck::measureVoltage() {

  pinMode(ctrlPin, OUTPUT);
  digitalWrite(ctrlPin, LOW);
  delay(10);

  analogReadResolution(12);
  analogSetPinAttenuation(adcPin, ADC_11db);

  uint32_t rawSum = 0;
  for (int i = 0; i < 10; i++) {
    rawSum += analogRead(adcPin);
    delay(1);
  }
  float raw = rawSum / 10.0;

  digitalWrite(ctrlPin, HIGH);

  float voltage = (raw / 4095.0) * 3.3 * 4.9;
  return (uint16_t)(voltage * 1000);
}

void BatteryCheck::forceUpdate() {

  Serial.println("Start update battery ...");

  uint16_t mvolts = measureVoltage();

  if (mvolts >= 4150) {
    cachedPercentage = 100;
  } else if (mvolts <= 3300) {
    cachedPercentage = 0;
  } else if (mvolts >= 4000) {
    cachedPercentage = 85 + (mvolts - 4000) * 15 / 150;
  } else if (mvolts >= 3800) {
    cachedPercentage = 60 + (mvolts - 3800) * 25 / 200;
  } else if (mvolts >= 3600) {
    cachedPercentage = 25 + (mvolts - 3600) * 35 / 200;
  } else if (mvolts >= 3400) {
    cachedPercentage = 5 + (mvolts - 3400) * 20 / 200;
  } else {
    cachedPercentage = (mvolts - 3300) * 5 / 100;
  }

  Serial.println("Battery update done!");
}

void BatteryCheck::update(unsigned long currentMillis) {

  if (lastCheckTime == 0 || (currentMillis - lastCheckTime >= checkInterval)) {
    lastCheckTime = currentMillis;
    forceUpdate();
  }
}

uint8_t BatteryCheck::getPercentage() {

  if (cachedPercentage == 0 && lastCheckTime == 0) {
    forceUpdate();
    lastCheckTime = millis();
  }
  return cachedPercentage;
}

uint16_t BatteryCheck::getVoltage() {
  return measureVoltage();
}