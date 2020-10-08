#include "Sensor.h"


Sens_val Sensor::sens = {0};
bool Sensor::systemSleep = true;

uint32_t (*Sensor::getTick)(void) = NULL;
void (*Sensor::halt)(uint32_t t) = NULL;

void Sensor::clearAllSensor(void){
    sens = ( Sens_val ) {
        .water_temperature  = 0,
        .air_temperature    = 0,
        .humidity           = 0,
        .pH                 = 0,
        .conductivity       = 0,
        .salinity           = 0,
        .tds                = 0,
        .specificOfGravity = 0,
        .DO2_mgl        = 0,
        .DO2_percent    = 0,
    };
}

void Sensor::setup(void){
    clearAllSensor();
    // default is sleep
    systemSleep = true;
}

void Sensor::setSleep(bool sleep){
    systemSleep = sleep;
}

bool Sensor::isSleep(void){
    return systemSleep;
}

void Sensor::attachGetTickCallback(uint32_t (*getTick)(void))
{
    Sensor::getTick = getTick;
}

void Sensor::attachDelayCallback(void (*halt)(uint32_t t))
{
    Sensor::halt = halt;
}

void Sensor::waterParamInfo(void)
{
    Serial.println("Water parameter Information");
    Serial.println(" pH : " + String(sens.pH,2));
    Serial.println(" DO : " + String(sens.DO2_mgl, 2));
    Serial.println(" EC : " + String(sens.conductivity, 2));
    Serial.println(" Temp : " + String(sens.water_temperature, 2));
    Serial.flush();
}