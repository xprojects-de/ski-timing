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
#define BUFFER_SIZE 64
#define TX_QUEUE_DEPTH 4
#define RX_QUEUE_DEPTH 10

#define LORA_PREFIX "ST-"
#define PREFIX_LEN 3

#define BURST_COUNT 3

static RadioEvents_t RadioEvents;

volatile bool _radioBusy = false;
volatile bool _rxActive = false;


SemaphoreHandle_t Lora::_dataMutex = NULL;
QueueHandle_t Lora::_txQueue = NULL;
QueueHandle_t Lora::_rxQueue = NULL;
volatile uint64_t Lora::_rxTimestamp = 0;

static void loraTaskWorker(void *parameter) {
  Lora *loraInstance = static_cast<Lora *>(parameter);
  for (;;) {
    loraInstance->proccess();
    vTaskDelay(pdMS_TO_TICKS(5));
  }
}

Lora::Lora() {
  if (_dataMutex == NULL) {
    _dataMutex = xSemaphoreCreateMutex();
  }
  if (_txQueue == NULL) {
    _txQueue = xQueueCreate(TX_QUEUE_DEPTH, BUFFER_SIZE);
  }
  if (_rxQueue == NULL) {
    _rxQueue = xQueueCreate(RX_QUEUE_DEPTH, sizeof(LoraRxPacket));
  }
}

Lora::~Lora() {

  if (_taskHandle != NULL) {
    vTaskDelete(_taskHandle);
    _taskHandle = NULL;
  }

  if (_dataMutex != NULL) {
    vSemaphoreDelete(_dataMutex);
    _dataMutex = NULL;
  }

  if (_txQueue != NULL) {
    vQueueDelete(_txQueue);
    _txQueue = NULL;
  }

  if (_rxQueue != NULL) {
    vQueueDelete(_rxQueue);
    _rxQueue = NULL;
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

void Lora::sendData(const char *data, bool burst) {
  if (_txQueue == NULL) return;

  char txBuf[BUFFER_SIZE];
  snprintf(txBuf, BUFFER_SIZE, "%s%s", LORA_PREFIX, data);

  if (burst) {

    int sentCount = 0;
    for (int i = 0; i < BURST_COUNT; i++) {
      if (xQueueSend(_txQueue, txBuf, pdMS_TO_TICKS(100)) == pdTRUE) {
        sentCount++;
      } else {
        Serial.printf("[LORA] Burst %d/%d: Queue full!\n", i + 1, BURST_COUNT);
        break;
      }
    }
    Serial.printf("[LORA] Burst: %d/%d packages put to queue\n", sentCount, BURST_COUNT);
  } else {
    if (xQueueSend(_txQueue, txBuf, 0) != pdTRUE) {
      Serial.println("[LORA] WARNING: TX queue full, message dropped!");
    }
  }
}

bool Lora::tryGetPacket(char *buffer, size_t bufferSize, int16_t &size, int16_t &rssi, int8_t &snr) {
  if (_rxQueue == NULL) return false;

  LoraRxPacket packet;
  if (xQueueReceive(_rxQueue, &packet, 0) == pdTRUE) {
    size_t copyLen = (packet.size < (int16_t)(bufferSize - 1)) ? packet.size : (bufferSize - 1);
    memcpy(buffer, packet.data, copyLen);
    buffer[copyLen] = '\0';
    size = (int16_t)copyLen;
    rssi = packet.rssi;
    snr = packet.snr;
    return true;
  }

  return false;
}

void Lora::proccess() {

  if (!_radioBusy) {
    char txBuf[BUFFER_SIZE];
    if (_txQueue != NULL && xQueueReceive(_txQueue, txBuf, 0) == pdTRUE) {
      _radioBusy = true;
      _rxActive = false;
      Radio.Send((uint8_t *)txBuf, strlen(txBuf));
    } else if (!_rxActive) {
      _rxActive = true;
      Radio.Rx(0);
    }
  }
  Radio.IrqProcess();
}

void Lora::OnTxDone() {
  _radioBusy = false;
}

void Lora::OnTxTimeout() {
  _radioBusy = false;
}

void Lora::OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr) {

  _rxTimestamp = (uint64_t)esp_timer_get_time();

  if (size >= PREFIX_LEN && memcmp(payload, LORA_PREFIX, PREFIX_LEN) == 0) {

    uint16_t actualDataSize = size - PREFIX_LEN;
    uint16_t copySize = (actualDataSize < BUFFER_SIZE) ? actualDataSize : (BUFFER_SIZE - 1);

    if (_rxQueue != NULL) {
      LoraRxPacket packet;
      memcpy(packet.data, payload + PREFIX_LEN, copySize);
      packet.data[copySize] = '\0';
      packet.size = (int16_t)copySize;
      packet.rssi = rssi;
      packet.snr = snr;
      packet.rxTimestamp = _rxTimestamp;

      if (xQueueSend(_rxQueue, &packet, 0) != pdTRUE) {
        Serial.println("[LORA] RX Queue full! skipp package.");
      }
    }
  }

  _rxActive = false;
  _radioBusy = false;
}

void Lora::OnRxTimeout() {
  _rxActive = false;
  _radioBusy = false;
}

void Lora::OnRxError() {
  _rxActive = false;
  _radioBusy = false;
}
