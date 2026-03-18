#include "lora.h"

#define RF_FREQUENCY                                865000000 // Hz

#define TX_OUTPUT_POWER                             5         // dBm

#define LORA_BANDWIDTH                              0         // [0: 125 kHz,
                                                              //  1: 250 kHz,
                                                              //  2: 500 kHz,
                                                              //  3: Reserved]
#define LORA_SPREADING_FACTOR                       7         // [SF7..SF12]
#define LORA_CODINGRATE                             1         // [1: 4/5,
                                                              //  2: 4/6,
                                                              //  3: 4/7,
                                                              //  4: 4/8]
#define LORA_PREAMBLE_LENGTH                        8         // Same for Tx and Rx
#define LORA_SYMBOL_TIMEOUT                         0         // Symbols
#define LORA_FIX_LENGTH_PAYLOAD_ON                  false
#define LORA_IQ_INVERSION_ON                        false


#define RX_TIMEOUT_VALUE                            1000
#define BUFFER_SIZE                                 30 // Define the payload size here

char txpacket[BUFFER_SIZE];
char rxpacket[BUFFER_SIZE];

static RadioEvents_t RadioEvents;

int16_t txNumber;
bool sendTrigger = false;
bool lora_idle = true;
int16_t rssi,rxSize;

Lora::Lora() {
}

Lora::~Lora() {}

void Lora::preInit() {
    Mcu.begin(HELTEC_BOARD,SLOW_CLK_TPYE);
}

void Lora::init() {
    
    txNumber=0;
    rssi=0;

    RadioEvents.TxDone = Lora::OnTxDone;
    RadioEvents.TxTimeout = Lora::OnTxTimeout;
    RadioEvents.RxDone = Lora::OnRxDone;

    Radio.Init( &RadioEvents );
    Radio.SetChannel( RF_FREQUENCY );
    Radio.SetTxConfig( MODEM_LORA, TX_OUTPUT_POWER, 0, LORA_BANDWIDTH,
                                   LORA_SPREADING_FACTOR, LORA_CODINGRATE,
                                   LORA_PREAMBLE_LENGTH, LORA_FIX_LENGTH_PAYLOAD_ON,
                                   true, 0, 0, LORA_IQ_INVERSION_ON, 3000 );

    Radio.SetRxConfig( MODEM_LORA, LORA_BANDWIDTH, LORA_SPREADING_FACTOR,
                                   LORA_CODINGRATE, 0, LORA_PREAMBLE_LENGTH,
                                   LORA_SYMBOL_TIMEOUT, LORA_FIX_LENGTH_PAYLOAD_ON,
                                   0, true, 0, 0, LORA_IQ_INVERSION_ON, true );

}

void Lora::send() {

  if(sendTrigger == false) {

    txNumber += 1;
    sprintf(txpacket,"Hello world number %d",txNumber);

    sendTrigger = true;

  }

}

void Lora::receive() {

}

void Lora::proccess() {

  if(lora_idle == true) {

    if(sendTrigger == true) {
      Radio.Send( (uint8_t *)txpacket, strlen(txpacket) );
      lora_idle = false;
    } 

  }

  Radio.IrqProcess();

}

void Lora::OnTxDone()
{
  lora_idle = true;
  sendTrigger = false;
}

void Lora::OnTxTimeout()
{
    Radio.Sleep( );
    lora_idle = true;
    sendTrigger = false;
}

void Lora::OnRxDone( uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr )
{
    rssi=rssi;
    rxSize=size;
    memcpy(rxpacket, payload, size );
    rxpacket[size]='\0';
    Radio.Sleep( );

    Serial.printf("\r\nreceived packet \"%s\" with Rssi %d , length %d\r\n",rxpacket,rssi,rxSize);
    Serial.println("wait to send next packet");

    lora_idle = true;
}

