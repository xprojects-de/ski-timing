#include "batteryCheck.h"

BatteryCheck::BatteryCheck(uint8_t adc_pin, uint8_t ctrl_pin, unsigned long interval_ms)
  : adcPin(adc_pin), ctrlPin(ctrl_pin), cachedVoltage(0), cachedPercentage(0),
    checkInterval(interval_ms) {}

void BatteryCheck::init() {

  pinMode(ctrlPin, OUTPUT);
  digitalWrite(ctrlPin, LOW);

  analogReadResolution(12);
  analogSetPinAttenuation(adcPin, ADC_11db);

  forceUpdate();
}

uint16_t BatteryCheck::measureVoltage() {

  // The battery voltage is divided by a resistor divider before reaching
  // the ADC pin.  The factor 4.9 reconstructs the original battery voltage
  // from the ADC reading in millivolts.
  // (Typical divider: 100 kΩ / 390 kΩ → ratio ≈ 4.9)
  static const float DIVIDER_FACTOR = 4.9f;

  digitalWrite(ctrlPin, HIGH);
  delay(10);

  uint32_t mvSum = 0;
  for (int i = 0; i < 10; i++) {
    mvSum += analogReadMilliVolts(adcPin);
    delay(1);
  }
  float avgMv = mvSum / 10.0f;

  digitalWrite(ctrlPin, LOW);

  float voltage = avgMv * DIVIDER_FACTOR;
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

void BatteryCheck::taskWorker(void *parameter) {
  BatteryCheck *self = static_cast<BatteryCheck *>(parameter);
  for (;;) {
    vTaskDelay(pdMS_TO_TICKS(self->checkInterval));
    self->forceUpdate();
  }
}

void BatteryCheck::startTask(UBaseType_t priority, BaseType_t coreID) {
  if (_taskHandle == NULL) {
    xTaskCreatePinnedToCore(
      taskWorker,
      "BatteryTask",
      2048,
      this,
      priority,
      &_taskHandle,
      coreID);
  }
}

uint8_t BatteryCheck::getPercentage() {
  return cachedPercentage;
}

uint16_t BatteryCheck::getVoltage() {
  return cachedVoltage;
}