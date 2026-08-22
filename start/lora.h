#ifndef LORA_H
#define LORA_H

#include "LoRaWan_APP.h"
#include <freertos/semphr.h>
#include <freertos/queue.h>
#include <esp_timer.h>

class Lora {

private:
  TaskHandle_t _taskHandle = NULL;
  static SemaphoreHandle_t _dataMutex;
  static QueueHandle_t _txQueue;          // Thread-safe TX queue
  static volatile uint64_t _rxTimestamp;  // µs timestamp of last received packet (OnRxDone)

public:
  Lora();
  ~Lora();

  void preInit();
  void init();
  void startTask(UBaseType_t priority = 2, BaseType_t coreID = 1);
  void proccess();

  void sendData(const char* data, bool burst = false);

  bool tryGetPacket(char* buffer, size_t bufferSize, int16_t& size, int16_t& rssi, int8_t& snr);

  static uint64_t getRxTimestamp() {
    return _rxTimestamp;
  }

  static void OnTxDone();
  static void OnTxTimeout();
  static void OnRxDone(uint8_t* payload, uint16_t size, int16_t rssi, int8_t snr);
  static void OnRxTimeout();
  static void OnRxError();
};

#endif