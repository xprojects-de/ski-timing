#ifndef LORA_H
#define LORA_H

#include "LoRaWan_APP.h"

class Lora {

private:

public:
  Lora();
  ~Lora();

  void preInit();
  void init();
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