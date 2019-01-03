#include <Arduino.h>

// Feather9x_TX
// -*- mode: C++ -*-
// Example sketch showing how to create a simple messaging client (transmitter)
// with the RH_RF95 class. RH_RF95 class does not provide for addressing or
// reliability, so you should only use RH_RF95 if you do not need the higher
// level messaging abilities.
// It is designed to work with the other example Feather9x_RX

#include "pt/pt.h"
#include "pt/pt-sem.h"

#include "radio.h"
#include "debug.h"

#include "Queue.h"

#include "frame.h" //frame structure

#define MAX_QUEUE_SIZE 1
Queue<message> messageQueue(MAX_QUEUE_SIZE);

//frame stat
frame_stat message_stat;

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

#define RADIO_FREQ 433.0
#define RADIO_NODE_ADDRESS 0x01
#define RADIO_TX_POWER 23
#define LONG_RANGE 0
#define RADIO_GATEWAY_ADDRESS 0x00

#define PT_DELAY(pt,ms,tsVar) \
  tsVar = millis(); \
  PT_WAIT_UNTIL(pt, millis()-tsVar >= (ms));

struct pt ptSerialRx; //Reading data from serial port
struct pt ptRadioTx; //Transmitting data over LoRA

struct pt ptRadioRx; //dummy -- need to be compleated
struct pt ptSerialTx; //dummy -- need to be compleated

/**
 * Start Frame Delimiter for UART communication
 */
#define SERIAL_SFD 0x7E
#define SERIAL_TIMEOUT 1 //second

#define IS_TIMEOUT( ms, tsVar ) \
  (millis() - tsVar) > ms

#define SERIAL_WAIT( n ) \
  Serial.available() >= n


/***********************************************
 * 
 */
PT_THREAD(taskSerialRx(struct pt* pt)) {
  static time_t ts=0;

  static uint8_t len, sfd, cs, i;

  //struct message
  static message Message;
  static message *myMessage = &Message;

  // printf(Serial,"Starting PT_THREAD(taskSerialRx)\r\n");

  PT_BEGIN(pt);

  for (;;) {

    //checking to find Start Frame Delimiter 0x7E
    while (true) {
      PT_WAIT_UNTIL(pt,Serial.available());
      sfd = Serial.read();
      if (sfd==SERIAL_SFD)
        break;
    }

    //waiting for next 1 bytes or Timeout occurs to extract the message
    ts = millis();
    PT_WAIT_UNTIL(pt, (Serial.available() >= 1) || IS_TIMEOUT(SERIAL_TIMEOUT*1000, ts));
    if ( Serial.available() < 1 ) continue; //event: SERIAL TIMEOUT

    //get msgType 
    myMessage->framehdr.msgType = Serial.read();

    //Case: msgType == FRAME_TYPE_MESSAGE 
    if (myMessage->framehdr.msgType == frame_types::FRAME_TYPE_MESSAGE) {
      myMessage->framehdr.len = Serial.read();
      len = myMessage->framehdr.len;

      //waiting for next len bytes || Timeout occurs
      ts = millis();
      PT_WAIT_UNTIL(pt, (Serial.available() >= len) || IS_TIMEOUT(SERIAL_TIMEOUT*1000, ts));
      if ( Serial.available() < len ) continue; //event: SERIAL TIMEOUT

      //Read next len bytes
      if (len) 
        Serial.readBytes((char *)myMessage->payload, len);

      message_stat.rx++;

      //Waiting before Queue is not full -- push the data messageQueue
      PT_WAIT_UNTIL(pt, messageQueue.count() < MAX_QUEUE_SIZE );
      messageQueue.push(*myMessage);
    }
    // case: other msgType // continue to read other // no handle 
    else {
      continue;
    }
  }
  PT_END(pt);

}

static inline void print_message(const message m) {

}

/***********************************************
 * 
 */
PT_THREAD(taskRadioTx(struct pt* pt)) {

  static uint8_t len=0;
  static uint8_t framehdrlen=0;
  static uint8_t sent_seq = 0;

  static message txMessage; //tx packet

  static time_t ts=0, duration=0;

  // printf(Serial,"Starting PT_THREAD(taskRadioTx)\r\n");

  PT_BEGIN(pt);
  for (;;) {
    PT_WAIT_UNTIL(pt,radio.send_done());

    //wating for data in Queue becomes available // produce by taskSerial
    PT_WAIT_UNTIL(pt, messageQueue.count() > 0);

    txMessage = messageQueue.pop();
    txMessage.framehdr.seq = sent_seq;
    sent_seq++;

    //Case: msgType == FRAME_TYPE_MESSAGE 
    framehdrlen  = sizeof(struct framehdr);
  
    if (txMessage.framehdr.msgType == frame_types::FRAME_TYPE_MESSAGE) {
      len = txMessage.framehdr.len;
    
      // printf(Serial, "TX SEQ %u DataSize %d ", txMessage.framehdr.seq, len);
      // for (uint16_t i=0; i<len; i++)
      //   printf(Serial, "%c", txMessage.payload[i]);
      // printf(Serial, "\r\n");

      //sending the data over LoRA
      ts = millis();
      radio.send(RADIO_GATEWAY_ADDRESS, (uint8_t*)&txMessage, framehdrlen+len);
      printf(Serial, "TX SEQ %u sends \n", txMessage.framehdr.seq, duration, framehdrlen+len);
      PT_WAIT_UNTIL(pt, radio.send_done());
      duration = millis() - ts;
      printf(Serial, "TX SEQ %u sendtime %lu msecs pktsize %d", txMessage.framehdr.seq, duration, framehdrlen+len);
      printf(Serial, "\r\n");
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
PT_THREAD(taskSerialTx(struct pt* pt)) {
  // printf(Serial,"Starting PT_THREAD(taskSerialTx)\r\n");
  PT_BEGIN(pt);

  for(;;) {

  }

  PT_END(pt);
}

/***********************************************
 * 
 */
PT_THREAD(taskRadioRx(struct pt* pt)) {
  // printf(Serial,"Starting PT_THREAD(taskRadioRx)\r\n");
  PT_BEGIN(pt);

  for(;;) {
    
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

  //initial pt
  PT_INIT(&ptSerialRx);
  PT_INIT(&ptRadioTx);

  //initial dummy pt
  PT_INIT(&ptSerialTx);
  PT_INIT(&ptRadioRx);
}
 
void loop()
{

  //Running pthread
  taskSerialRx(&ptSerialRx);
  taskRadioTx(&ptRadioTx);

  //Running dummy thread
  // taskSerialTx(&ptSerialTx);
  // taskRadioRx(&ptRadioRx);

}