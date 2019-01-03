#include <Arduino.h>

// Feather9x_TX
// -*- mode: C++ -*-
// Example sketch showing how to create a simple messaging client (transmitter)
// with the RH_RF95 class. RH_RF95 class does not provide for addressing or
// reliability, so you should only use RH_RF95 if you do not need the higher
// level messaging abilities.
// It is designed to work with the other example Feather9x_RX
#include "pt/pt.h"

#include "radio.h"
#include "debug.h"

#include "Queue.h"
#include "frame.h"
#define MAX_QUEUE_SIZE 1
Queue<message> messageQueue(MAX_QUEUE_SIZE);

/**
 * Start Frame Delimiter for UART communication
 */
#define SERIAL_SFD 0x7E
#define SERIAL_TIMEOUT 1 //second

#define RFM95_CS 8
#define RFM95_RST 4
#define RFM95_INT 3
 
#if defined(ESP8266)
  /* for ESP w/featherwing */ 
  #define RFM95_CS  2    // "E"
  #define RFM95_RST 16   // "D"
  #define RFM95_INT 15   // "B"
 
#elif defined(ESP32)  
  /* ESP32 feather w/wing */
  #define RFM95_RST     27   // "A"
  #define RFM95_CS      33   // "B"
  #define RFM95_INT     12   //  next to A
 
#elif defined(NRF52)  
  /* nRF52832 feather w/wing */
  #define RFM95_RST     7   // "A"
  #define RFM95_CS      11   // "B"
  #define RFM95_INT     31   // "C"
  
#elif defined(TEENSYDUINO)
  /* Teensy 3.x w/wing */
  #define RFM95_RST     9   // "A"
  #define RFM95_CS      10   // "B"
  #define RFM95_INT     4    // "C"
#endif
 

Radio radio;
uint8_t sent_seq = 0;
#define RADIO_FREQ 433.0
#define RADIO_NODE_ADDRESS 0x00
#define RADIO_TX_POWER 23
#define LONG_RANGE 0
#define RADIO_GATEWAY_ADDRESS 0x00

#define MAX_PAYLOAD_SIZE 255 //should be 255 //for 256, it will be overflowed uint8_t size
struct pt ptRadioRx;
struct pt ptSerialTx;

/***********************************************
 *  
 */
static inline void printRxInfo(uint8_t seq, uint8_t len, int16_t last_rssi, time_t waiting_time) {
  Serial.write(SERIAL_SFD);
  printf(Serial, "RX_SEQ %d LEN %d RSSI %d TIME %u", seq, len, last_rssi, waiting_time);
  printf(Serial, "\r\n");
}

static inline void printData() {
  Serial.write(SERIAL_SFD);
}

/***********************************************
 * 
 */
PT_THREAD(taskSerialTx(struct pt* pt)) {
  static uint8_t len;
  static message rxMessage;
  static message *pRxMessage = &rxMessage;

  // printf(Serial,"Starting PT_THREAD(taskSerialTx)\r\n");
  PT_BEGIN(pt);

  for (;;) {
    //wating for data in Queue becomes available // produce by taskSerial
    PT_WAIT_UNTIL(pt, messageQueue.count() > 0);
    rxMessage = messageQueue.pop();

    if (rxMessage.framehdr.msgType == frame_types::FRAME_TYPE_MESSAGE) {
      Serial.write(SERIAL_SFD);
      // Serial.write("D");
      Serial.write(rxMessage.framehdr.msgType); //msgType
      Serial.write(rxMessage.framehdr.len); //len
      len = rxMessage.framehdr.len;
      for (uint16_t i=0; i<len; i++) //all data
        Serial.write(rxMessage.payload[i]);
      Serial.flush();
      //printf(Serial, "\r\n");
    }
    else {
      continue;
    }
  }

  PT_END(pt);
}


/***********************************************
 * 
 */
PT_THREAD(taskRadioRx(struct pt* pt)) {
  static Address from;
  static uint8_t len;

  static message rxMessage;
  static message *pRxMessage = &rxMessage;

  static time_t ts=millis(), waiting_time=0;

  // printf(Serial,"Starting PT_THREAD(taskRadioRx)\r\n");

  PT_BEGIN(pt);
  for (;;) {

      PT_WAIT_UNTIL(pt,radio.available());

      waiting_time = millis() - ts; //calculate the waiting time
      ts = millis(); //keep current timestamp

      len = MAX_PAYLOAD_SIZE;
      radio.recv(&from, (uint8_t *)&rxMessage, &len);
  
      // printRxInfo(rxMessage.framehdr.seq, len, radio.last_rssi, waiting_time);

      //Case: msgType == FRAME_TYPE_MESSAGE 
      if (rxMessage.framehdr.msgType == frame_types::FRAME_TYPE_MESSAGE) {
        PT_WAIT_UNTIL(pt, messageQueue.count() < MAX_QUEUE_SIZE );
        messageQueue.push(rxMessage);
      }
      // case: other msgType // continue to read other // no handle 
      else {
        continue;
      }
  }
  PT_END(pt);
}

/***********************************************
 * 
 */
void init_radio() {
  printf(Serial,"Initializing radio...\r\n");
  if (!radio.init(
      RADIO_NODE_ADDRESS, //NodeAddress
      RADIO_FREQ, //Freq
      RADIO_TX_POWER,
      LONG_RANGE))
  {
    printf(Serial, "failed.\r\n");
  }
  printf(Serial, "successful; NodeId = 0x%02X\r\n", RADIO_NODE_ADDRESS);
}
 
/***********************************************
 * 
 */
void setup() 
{
  pinMode(PIN_RFM95_CS,OUTPUT);
  digitalWrite(PIN_RFM95_CS,HIGH);

  Serial.begin(115200);
  while (!Serial)
    ; // wait for serial port to connect. Needed for native USB port only
  
  printf(Serial,"Initialized RADIO!\r\n");
  printf(Serial,"NodeID = 0x%02X\r\n", RADIO_NODE_ADDRESS);

  init_radio();

  PT_INIT(&ptRadioRx);   //initial pt -- for radio handler
  PT_INIT(&ptSerialTx);     //initial pt -- for serial handler
}
 
void loop()
{
  taskRadioRx(&ptRadioRx); //Thread for handling the received LORA packet
  taskSerialTx(&ptSerialTx); //Thread for handling the received LORA packet
}