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

void setup() {
  setup_bsp();
  Serial.begin(115200);
  while (!Serial)
  {
    ; // wait for serial port to connect. Needed for native USB, on LEONARDO, MICRO, YUN, and other 32u4 based boards.
  }
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
              128,
              NULL,
              1,
              NULL);
  // vTaskStartScheduler();
}

void loop() {
  // vTaskDelay(500);
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
  for (;;)
  {
    Serial.println("======== Tasks status ========");
    Serial.print("Tick count: ");
    Serial.print(xTaskGetTickCount());
    Serial.print(", Task count: ");
    Serial.print(uxTaskGetNumberOfTasks());

    Serial.println();
    Serial.println();

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

    Serial.println();

    vTaskDelay(5000);// / portTICK_PERIOD_MS);
  }
}

/* 
 * Blink task. 
 * See Blink_AnalogRead example. 
 */
void TaskBlink(void *pvParameters)
{
  // (void)pvParameters;
  pinMode(LED_BUILTIN, OUTPUT);
  Sensor::attachDelayCallback(system_halt);
  Sensor::water::initSensorBoard();

  uint32_t time_reading = millis();
  // Sensor::water::setup();
  for (;;)
  {
    Serial.println("reading sensor");
    if ( millis() - time_reading <= 1000 )
    {
      Serial.println("reading sensor");
      Sensor::water::app();
      time_reading = millis();
    }
    

    digitalWrite(LED_BUILTIN, HIGH);
    vTaskDelay(250 / portTICK_PERIOD_MS);
    digitalWrite(LED_BUILTIN, LOW);
    vTaskDelay(250 / portTICK_PERIOD_MS);
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