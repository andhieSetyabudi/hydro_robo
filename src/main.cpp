#include "main.h"
#include "Arduino.h"
#include "board_io.h"
#include <Arduino_FreeRTOS.h>

// Modules
// ============= serial data query
#include "serial_com.h"
// ============= sensor
#include "Sensor.h"
#include <Ezo_uart.h>

TaskHandle_t taskBlinkHandle;

void system_halt(uint32_t t)
{
  vTaskDelay(t / portTICK_PERIOD_MS);
}

bool systemReady;

void setup() {
  setup_bsp();
  serial_com::setHalt(system_halt);
  serial_com::setup();

  if ( !initializeMemory() )
    resetMemory();
  else 
    Serial.println("load memory success");
  /**
   * Task creation
   */
  xTaskCreate(TaskBlink, // Task function
              "Blink",   // Task name
              1024,       // Stack size
              NULL,
              2,                 // Priority
              NULL); // Task handler

  xTaskCreate(TaskSerial,
              "Serial",
              2048,
              NULL,
              1,
              NULL);
  // vTaskStartScheduler();
}

void loop() {
  // uint32_t time_ = 60*15*1000;
  // vTaskDelay(1000 / portTICK_PERIOD_MS);
  // put your main code here, to run repeatedly:
//   set_sleep_mode(SLEEP_MODE_IDLE);

//   ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
//   {
//     sleep_enable();

// #if defined(BODS) && defined(BODSE) // Only if there is support to disable the brown-out detection.
//     sleep_bod_disable();
// #endif
//   }
//   sleep_cpu(); // good night.

//   // Ugh. I've been woken up. Better disable sleep mode.
//   sleep_disable();
}

void TaskLed(void *pvParameters)
{
  (void) pvParameters;
  pinMode(LED_BUILTIN, OUTPUT);

  for (;;) {
    
    /**
     * Take the semaphore.
     * https://www.freertos.org/a00122.html
     */
    
    vTaskDelay(10);
  }
}

void TaskSerial(void *pvParameters)
{
  (void)pvParameters;
  while(!systemReady) vTaskDelay(50);
  for (;;)
  {
    // Serial.println("======== Tasks status ========");
    // Serial.print("Tick count: ");
    // Serial.print(xTaskGetTickCount());
    // Serial.print(", Task count: ");
    // Serial.print(uxTaskGetNumberOfTasks());

    // Serial.println();
    // Serial.println();

    // Serial task status
    // Serial.print("- TASK ");
    // Serial.print(pcTaskGetName(NULL)); // Get task name without handler https://www.freertos.org/a00021.html#pcTaskGetName
    // Serial.print(", High Watermark: ");
    // Serial.print(uxTaskGetStackHighWaterMark(NULL)); // https://www.freertos.org/uxTaskGetStackHighWaterMark.html

    // // TaskHandle_t taskSerialHandle = xTaskGetCurrentTaskHandle(); // Get current task handle. https://www.freertos.org/a00021.html#xTaskGetCurrentTaskHandle

    // Serial.println();

    // Serial.print("- TASK ");
    // Serial.print(pcTaskGetName(taskBlinkHandle)); // Get task name with handler
    // Serial.print(", High Watermark: ");
    // Serial.print(uxTaskGetStackHighWaterMark(taskBlinkHandle));
    // Serial.println();

    // Serial.println();
    serial_com::app();
    vTaskDelay(100);// / portTICK_PERIOD_MS);
  }
}

/* 
 * Blink task. 
 * See Blink_AnalogRead example. 
 */
void TaskBlink(void *pvParameters)
{
  (void)pvParameters;
  pinMode(LED_BUILTIN, OUTPUT);
  Sensor::attachDelayCallback(system_halt);
  Sensor::water::initSensorBoard();
  unsigned long time_reading = millis();
  systemReady = true;
      // Sensor::water::setup();
  for (;;)
  {
    // Serial.println("reading sensor");
    if ( millis() - time_reading >= 200 )
    {
      Sensor::water::app();
      time_reading = millis();
    }
    vTaskDelay(50 / portTICK_PERIOD_MS);
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
  }
}

void TaskSensor(void *pvParameters)
{
  // sensor setup
  TickType_t xLastSensTime;
  for(;;)
  {
    // sensor looping
    vTaskDelayUntil(&xLastSensTime, (500 / portTICK_PERIOD_MS));
  }
}