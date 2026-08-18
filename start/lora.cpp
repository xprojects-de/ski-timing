#include "lora.h"

#define RF_FREQUENCY 868100000  // Hz

#define TX_OUTPUT_POWER 14  // dBm

#define LORA_BANDWIDTH 0          // [0: 125 kHz, \
                                 //  1: 250 kHz, \
                                 //  2: 500 kHz, \
                                 //  3: Reserved]
#define LORA_SPREADING_FACTOR 10  // [SF7..SF12]
#define LORA_CODINGRATE 4         // [1: 4/5, \
                                 //  2: 4/6, \
                                 //  3: 4/7, \
                                 //  4: 4/8]
#define LORA_PREAMBLE_LENGTH 8    // Same for Tx and Rx
#define LORA_SYMBOL_TIMEOUT 0     // Symbols
#define LORA_FIX_LENGTH_PAYLOAD_ON false
#define LORA_IQ_INVERSION_ON false


#define RX_TIMEOUT_VALUE 1000
#define BUFFER_SIZE 30

#define LORA_PREFIX "ST-"
#define PREFIX_LEN 3

// #define LORA_SYNCWORD 0x19

static RadioEvents_t RadioEvents;

char _rxBuffer[BUFFER_SIZE];
char _txBuffer[BUFFER_SIZE];
int16_t _rxSize = 0;
int16_t _lastRssi = 0;
int8_t _lastSnr = 0;

bool _sendRequested = false;
bool _packetReceived = false;
bool _radioBusy = false;

static void loraTaskWorker(void *parameter) {
  Lora *loraInstance = static_cast<Lora *>(parameter);
  for (;;) {
    loraInstance->proccess();
    vTaskDelay(pdMS_TO_TICKS(1));
  }
}

Lora::Lora() {}

Lora::~Lora() {
  if (_taskHandle != NULL) {
    vTaskDelete(_taskHandle);
    _taskHandle = NULL;
  }
}

void Lora::preInit() {
  Mcu.begin(HELTEC_BOARD, SLOW_CLK_TPYE);
}

void Lora::init() {

  RadioEvents.TxDone = Lora::OnTxDone;
  RadioEvents.TxTimeout = Lora::OnTxTimeout;
  RadioEvents.RxDone = Lora::OnRxDone;
  RadioEvents.RxTimeout = Lora::OnRxTimeout;
  RadioEvents.RxError = Lora::OnRxError;

  Radio.Init(&RadioEvents);

  Radio.SetPublicNetwork(false);
  // Radio.SetSyncWord(LORA_SYNCWORD);

  Radio.SetChannel(RF_FREQUENCY);

  Radio.SetTxConfig(MODEM_LORA, TX_OUTPUT_POWER, 0, LORA_BANDWIDTH,
                    LORA_SPREADING_FACTOR, LORA_CODINGRATE,
                    LORA_PREAMBLE_LENGTH, LORA_FIX_LENGTH_PAYLOAD_ON,
                    true, 0, 0, LORA_IQ_INVERSION_ON, 3000);

  Radio.SetRxConfig(MODEM_LORA, LORA_BANDWIDTH, LORA_SPREADING_FACTOR,
                    LORA_CODINGRATE, 0, LORA_PREAMBLE_LENGTH,
                    LORA_SYMBOL_TIMEOUT, LORA_FIX_LENGTH_PAYLOAD_ON,
                    0, true, 0, 0, LORA_IQ_INVERSION_ON, true);

  _radioBusy = false;
}

void Lora::startTask(UBaseType_t priority, BaseType_t coreID) {
  if (_taskHandle == NULL) {
    xTaskCreatePinnedToCore(
      loraTaskWorker,
      "LoraTask",
      4096,
      this,
      priority,
      &_taskHandle,
      coreID);
  }
}

void Lora::sendData(const char *data) {

  if (!_radioBusy) {
    snprintf(_txBuffer, BUFFER_SIZE, "%s%s", LORA_PREFIX, data);
    _sendRequested = true;
  }
}

bool Lora::available() {
  return _packetReceived;
}

void Lora::getPacket(char *buffer, int16_t &size, int16_t &rssi, int8_t &snr) {

  if (_packetReceived) {

    memcpy(buffer, _rxBuffer, _rxSize);
    buffer[_rxSize] = '\0';
    size = _rxSize;
    rssi = _lastRssi;
    snr = _lastSnr;
    _packetReceived = false;
  }
}

void Lora::proccess() {

  if (!_radioBusy) {

    if (_sendRequested) {

      _radioBusy = true;
      _sendRequested = false;
      Radio.Send((uint8_t *)_txBuffer, strlen(_txBuffer));

    } else {
      Radio.Rx(0);
    }
  }
  Radio.IrqProcess();
}

void Lora::OnTxDone() {
  _radioBusy = false;
}

void Lora::OnTxTimeout() {
  //Radio.Sleep();
  _radioBusy = false;
}

void Lora::OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr) {

  if (size >= PREFIX_LEN && memcmp(payload, LORA_PREFIX, PREFIX_LEN) == 0) {

    uint16_t actualDataSize = size - PREFIX_LEN;
    _rxSize = (actualDataSize < BUFFER_SIZE) ? actualDataSize : (BUFFER_SIZE - 1);

    memcpy(_rxBuffer, payload + PREFIX_LEN, _rxSize);
    _rxBuffer[_rxSize] = '\0';

    _lastRssi = rssi;
    _lastSnr = snr;
    _packetReceived = true;
  } else {
    _packetReceived = false;
  }

  _radioBusy = false;
}

void Lora::OnRxTimeout() {
  _radioBusy = false;
}

void Lora::OnRxError() {
  _radioBusy = false;
}
