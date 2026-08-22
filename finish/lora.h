#ifndef LORA_H
#define LORA_H

#include "LoRaWan_APP.h"
#include <freertos/queue.h>
#include <esp_timer.h>

struct LoraRxPacket {
  char data[64];
  int16_t size;
  int16_t rssi;
  int8_t snr;
  uint64_t rxTimestamp;
};

class Lora {

private:
  TaskHandle_t _taskHandle = NULL;
  static QueueHandle_t _txQueue;          // Thread-safe TX queue
  static QueueHandle_t _rxQueue;          // Thread-safe RX queue (FIFO)
  static volatile uint64_t _rxTimestamp;  // µs timestamp of last received packet (OnRxDone)

public:
  Lora();
  ~Lora();

  void preInit();
  void init();
  void startTask(UBaseType_t priority = 2, BaseType_t coreID = 1);
  void process();

  void sendData(const char* data);

  bool tryGetPacket(char* buffer, size_t bufferSize, int16_t& size, int16_t& rssi, int8_t& snr, uint64_t& rxTimestamp);

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