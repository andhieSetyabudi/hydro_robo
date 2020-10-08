// #pragma once
#ifndef main_h_
#define main_h_

#include <Arduino.h>
#include <util/atomic.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>



void TaskLed(void *pvParameters);
void setup();
void loop();

void TaskBlink(void *pvParameters);
void TaskSerial(void *pvParameters);

void TaskSensor(void *pvParameters);
#endif