#ifndef LORA_H
#define LORA_H

#include "LoRaWan_APP.h"
#include <freertos/semphr.h>

class Lora {

private:
  TaskHandle_t _taskHandle = NULL;
  static SemaphoreHandle_t _dataMutex;

public:
  Lora();
  ~Lora();

  void preInit();
  void init();
  void startTask(UBaseType_t priority = 2, BaseType_t coreID = 1);
  void proccess();

  void sendData(const char* data);

  bool available();
  void getPacket(char* buffer, int16_t& size, int16_t& rssi, int8_t& snr);

  static void OnTxDone();
  static void OnTxTimeout();
  static void OnRxDone(uint8_t* payload, uint16_t size, int16_t rssi, int8_t snr);
  static void OnRxTimeout();
  static void OnRxError();
};

#endif