#ifndef __IO_H__
#define __IO_H__

#ifdef ARDUINO_SAMD_FEATHER_M0
#define PIN_VBAT A7
#define PIN_RFM95_CS  8
#define PIN_RFM95_RST 4
#define PIN_RFM95_INT 3
#endif

#ifdef ARDUINO_AVR_FEATHER32U4
#define PIN_VBAT A9
#define PIN_RFM95_CS  8
#define PIN_RFM95_RST 4
#define PIN_RFM95_INT 7
#endif

#define PIN_SD_SS     10
#define PIN_1PPS      11
#define PIN_GPS_EN    12

#define LED_ON()      digitalWrite(LED_BUILTIN,HIGH)
#define LED_OFF()     digitalWrite(LED_BUILTIN,LOW)
#define GPS_ON()      digitalWrite(PIN_GPS_EN,LOW)
#define GPS_OFF()     digitalWrite(PIN_GPS_EN,HIGH)
#define RADIO_ON()    radio.idle()
#define RADIO_OFF()   radio.sleep()

#define PT_SHORT_BLINK(pt,ts,on) \
        do { \
          digitalWrite(LED_BUILTIN,HIGH); \
          PT_DELAY(pt,on,ts); \
          digitalWrite(LED_BUILTIN,LOW); \
        } while (0);

#define SHORT_BLINK(on,off) \
        do { \
          digitalWrite(LED_BUILTIN,HIGH); \
          delay(on); \
          digitalWrite(LED_BUILTIN,LOW); \
          delay(off); \
        } while (0);

#endif
