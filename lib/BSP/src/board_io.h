// #pragma once
#ifndef _board_io_h_
#define _board_io_h_

#include "Arduino.h"
#include "Arduino_FreeRTOS.h"
#include "SoftwareSerial.h"

#ifndef ENABLE
    #define ENABLE  HIGH
#endif

#ifndef DISABLE
    #define DISABLE LOW
#endif

#define S1_tentacles_pin    6
#define S0_tentacles_pin    7

#define E1_tentacles_pin    5
#define E2_tentacles_pin    4

#define SET_PIN(pin)        digitalWrite(pin, HIGH)
#define CLEAR_PIN(pin)      digitalWrite(pin, LOW)



#define EZO_TX_PIN          10
#define EZO_RX_PIN          11

#define SERIAL_BAUD         115200

#define UniqueIDsize        9

struct timedateParam{
    uint8_t date, month, year;
};
struct deviceParam{

};

extern char serialDevice[UniqueIDsize+1];

void setup_bsp(void);

void tentacles_open_channel(uint8_t ch);


#endif