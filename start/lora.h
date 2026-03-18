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
  void send();
  void receive();
  void proccess();

  static void OnTxDone();
  static void OnTxTimeout();
  static void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr);
};

#endif