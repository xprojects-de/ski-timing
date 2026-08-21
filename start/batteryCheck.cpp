#include "batteryCheck.h"

BatteryCheck::BatteryCheck(uint8_t adc_pin, uint8_t ctrl_pin, unsigned long interval_ms)
  : adcPin(adc_pin), ctrlPin(ctrl_pin), cachedVoltage(0), cachedPercentage(0),
    lastCheckTime(0), checkInterval(interval_ms) {}

void BatteryCheck::init() {

  pinMode(ctrlPin, OUTPUT);
  digitalWrite(ctrlPin, LOW);

  analogReadResolution(12);
  analogSetPinAttenuation(adcPin, ADC_11db);

  forceUpdate();

  lastCheckTime = millis();
}

uint16_t BatteryCheck::measureVoltage() {

  digitalWrite(ctrlPin, HIGH);
  delay(10);

  uint32_t mvSum = 0;
  for (int i = 0; i < 10; i++) {
    mvSum += analogReadMilliVolts(adcPin);
    delay(1);
  }
  float avgMv = mvSum / 10.0;

  digitalWrite(ctrlPin, LOW);

  float voltage = avgMv * 4.9;
  return (uint16_t)voltage;
}

uint8_t BatteryCheck::voltageToPercentage(uint16_t mvolts) {

  if (mvolts >= 4150) {
    return 100;
  } else if (mvolts <= 3300) {
    return 0;
  } else if (mvolts >= 4000) {
    return 85 + (mvolts - 4000) * 15 / 150;
  } else if (mvolts >= 3800) {
    return 60 + (mvolts - 3800) * 25 / 200;
  } else if (mvolts >= 3600) {
    return 25 + (mvolts - 3600) * 35 / 200;
  } else if (mvolts >= 3400) {
    return 5 + (mvolts - 3400) * 20 / 200;
  } else {
    return (mvolts - 3300) * 5 / 100;
  }
}

void BatteryCheck::forceUpdate() {

  Serial.println("Start update battery ...");

  cachedVoltage = measureVoltage();
  cachedPercentage = voltageToPercentage(cachedVoltage);

  Serial.print("Battery: ");
  Serial.print(cachedVoltage);
  Serial.print(" mV / ");
  Serial.print(cachedPercentage);
  Serial.println(" %");
}

void BatteryCheck::update(unsigned long currentMillis) {

  if (lastCheckTime == 0 || (currentMillis - lastCheckTime >= checkInterval)) {
    lastCheckTime = currentMillis;
    forceUpdate();
  }
}

uint8_t BatteryCheck::getPercentage() {

  if (lastCheckTime == 0) {
    forceUpdate();
    lastCheckTime = millis();
  }
  return cachedPercentage;
}

uint16_t BatteryCheck::getVoltage() {

  if (lastCheckTime == 0) {
    forceUpdate();
    lastCheckTime = millis();
  }
  return cachedVoltage;
}