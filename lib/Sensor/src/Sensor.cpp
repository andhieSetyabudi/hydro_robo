#include "Sensor.h"
#include "board_io.h"

Sens_val Sensor::sens = {0};
bool Sensor::systemSleep = true;

uint32_t (*Sensor::getTick)(void) = NULL;
void (*Sensor::halt)(uint32_t t) = NULL;

StabilityDetector Sensor::pH_stable_;
StabilityDetector Sensor::DO_stable_;
StabilityDetector Sensor::EC_stable_;
StabilityDetector Sensor::water_temp_stable_;

StabilityDetector Sensor::pH_uncal_stable;
StabilityDetector Sensor::ec_uncal_stable;
StabilityDetector Sensor::do_uncal_stable;

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
        .airPressure_in_kpa = 0,
    };

    pH_stable_.resetValue();
    DO_stable_.resetValue();
    EC_stable_.resetValue();
    water_temp_stable_.resetValue();

    pH_uncal_stable.resetValue();
    ec_uncal_stable.resetValue();
    do_uncal_stable.resetValue();
}

void Sensor::setup(void){
    clearAllSensor();
    // set precision 
    pH_stable_.setPrecision(deviceParameter.pH_precision);
    DO_stable_.setPrecision(deviceParameter.DO_precision);
    EC_stable_.setPrecision(deviceParameter.EC_precision);
    water_temp_stable_.setPrecision(deviceParameter.water_temperature_precision);

    pH_uncal_stable.setPrecision(deviceParameter.pH_precision);
    ec_uncal_stable.setPrecision(deviceParameter.DO_precision);
    do_uncal_stable.setPrecision(deviceParameter.EC_precision);

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
    Serial.println(" EC : " + String(getConductivity(), 2));
    Serial.println(" Temp : " + String(sens.water_temperature, 2));
    Serial.flush();
}