#ifndef BATTERY_CHECK_H
#define BATTERY_CHECK_H

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

class BatteryCheck {

private:
  uint8_t adcPin;
  uint8_t ctrlPin;
  volatile uint16_t cachedVoltage;
  volatile uint8_t cachedPercentage;
  unsigned long checkInterval;
  TaskHandle_t _taskHandle = NULL;

  uint16_t measureVoltage();
  uint8_t voltageToPercentage(uint16_t mvolts);
  void forceUpdate();

  static void taskWorker(void *parameter);

public:

  BatteryCheck(uint8_t adc_pin = 1, uint8_t ctrl_pin = 37, unsigned long interval_ms = 60000);

  void init();
  void startTask(UBaseType_t priority = 1, BaseType_t coreID = 1);

  uint16_t getVoltage();
  uint8_t getPercentage();
};

#endif