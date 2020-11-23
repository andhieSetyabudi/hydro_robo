#include "board_io.h"
#include "Sensor.h"

uint32_t Sensor::air::lastSensAirTime = 0;

Ezo_uart Sensor::air::ezo_Module1(Serial2);

void Sensor::air::updateTimeMillis(void)
{
    if (Sensor::getTick == NULL)
        lastSensAirTime = millis();
    else
        lastSensAirTime = Sensor::getTick();
}

void Sensor::air::airDelay(uint32_t timeDelay)
{
    if (Sensor::halt == NULL)
        delay(timeDelay);
    else
        Sensor::halt(timeDelay);
}

void Sensor::air::setup(void)
{
    Serial2.begin(9600);
    Serial2.flush();
    Serial.println(F("Air Sensor Initializing"));
    airDelay(10);
    char buf_msg[20] = "";
    ezo_Module1.flush_rx_buffer();
    // ezo_Module1.send_cmd("Factory", NULL, 0);
    ezo_Module1.send_cmd("i", buf_msg, 19);
    if (strlen(buf_msg) > 1)
    {
        Serial.println("i : "+String(buf_msg));
        ezo_Module1.send_cmd("C,0", NULL, 0); //send the command to turn off continuous mode
                                             //in this case we arent concerned about waiting for the reply
        airDelay(100);
        ezo_Module1.send_cmd("*OK,0", NULL, 0); // turn of "OK" reply
    }
}

void Sensor::air::app(void)
{
    if (!ezo_Module1.send_read())
        return;
    sens.humidity = ezo_Module1.get_reading();
    Serial.println("hum : "+String(sens.humidity));
}