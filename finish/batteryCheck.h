#ifndef BATTERY_CHECK_H
#define BATTERY_CHECK_H

#include <Arduino.h>

class BatteryCheck {

private:
  uint8_t adcPin;
  uint8_t ctrlPin;
  uint16_t cachedVoltage;
  uint8_t cachedPercentage;
  unsigned long lastCheckTime;
  unsigned long checkInterval;

  uint16_t measureVoltage();
  uint8_t voltageToPercentage(uint16_t mvolts);

public:

  BatteryCheck(uint8_t adc_pin = 1, uint8_t ctrl_pin = 37, unsigned long interval_ms = 60000);

  void init();
  void update(unsigned long currentMillis);
  void forceUpdate();

  uint16_t getVoltage();
  uint8_t getPercentage();
};

#endif