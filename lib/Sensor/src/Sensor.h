
#ifndef _SENSOR_H_
#define _SENSOR_H_

#include "Arduino.h"
#include "SoftwareSerial.h"
#include "stdio.h"
#include "string.h"
#include "math.h"

#include "extra/stabilityDetector/StabilityDetector.h"
#include <Ezo_uart.h>

#ifndef TENTACLES_CH_NUM
    #define TENTACLES_CH_NUM 8
#endif
#ifndef NUM_OF_EZO
    #define NUM_OF_EZO  4
#endif

/**
 * Module Info contains :
 *                      - Module Name : 
 *                                      a. TP : Temperature     b. pH       c. EC : conductivity        d. DO : DissolvedOxygen
 *                      - Status : 
 *                                  IDLE,  No Responding, 
 **/
struct module_info{
    char name[10];
    uint8_t status;
    uint8_t ch;
};

struct Sens_val{
    float water_temperature,
            air_temperature,
            humidity,
            pH,
            conductivity,
            salinity,
            tds,
            specificOfGravity,
            DO2_mgl,
            DO2_percent,
            airPressure_in_kpa;
    // uncalibrated value
    float pH_uncal,
          ec_uncal,
          do_percent_uncal;
};

class Sensor{
    private : 
        static bool systemSleep;

        static Sens_val sens;
        // stability detector
        static StabilityDetector pH_stable_;
        static StabilityDetector DO_stable_;
        static StabilityDetector EC_stable_;
        static StabilityDetector water_temp_stable_;

        static StabilityDetector pH_uncal_stable;
        static StabilityDetector ec_uncal_stable;
        static StabilityDetector do_uncal_stable;

        static void clearAllSensor(void);
        static uint32_t (*getTick)(void);
        static void (*halt)  (uint32_t t);
        static void waterParamInfo(void);
    public:
        static void setup(void);

        static void attachGetTickCallback(uint32_t (*getTick)(void) );
        static void attachDelayCallback(void (*halt)(uint32_t t) );

        static void setSleep(bool sleep);
        static bool isSleep(void);

        static float getAirPressure(void)       { return sens.airPressure_in_kpa;}
        static float getWaterTemperature(void)  { return sens.water_temperature;}
        static bool  isWaterTempStable(void)    { return water_temp_stable_.isStable();}
        static float getWaterTemp_stdev(void)   { return water_temp_stable_.getDeviasionValue();}

        static float getpH(void)                { return sens.pH;}
        static bool  isPHStable(void)           { return pH_stable_.isStable();}
        static float getPH_stdev(void)          { return pH_stable_.getDeviasionValue();}

        static float getConductivity(void)      { return sens.conductivity;}
        static float getSalinity(void)          { return sens.salinity;}
        static float getSpecifivGravity(void)   { return sens.specificOfGravity;}
        static float getTDS(void)               { return sens.tds;}
        static bool  isConductivityStable(void) { return EC_stable_.isStable();}
        static float getEC_stdev(void)          { return EC_stable_.getDeviasionValue();}

        static float getDO_percent(void)        { return sens.DO2_percent;}
        static float getDO_mgl(void)            { return sens.DO2_mgl;}
        static bool  isDOStable(void)           { return DO_stable_.isStable();}
        static float getDO_stdev(void)          { return DO_stable_.getDeviasionValue();}


    // uncalibrate_param
        // pH
        static float getPH_uncal(void)              { return sens.pH_uncal;}
        static float getPH_uncal_stdev(void)        { return pH_uncal_stable.getDeviasionValue();}
        static float getPH_uncal_stableCount(void)  { return pH_uncal_stable.getStableCount();}
        static bool isPH_uncal_stable(void)         { return pH_uncal_stable.isStable();}
        // DO
        static float getDO_percent_uncal(void)      { return sens.do_percent_uncal;}
        static float getDO_uncal_stdev(void)        { return do_uncal_stable.getDeviasionValue();}
        static float getDO_uncal_stableCount(void)  { return do_uncal_stable.getStableCount();}
        static bool isDO_uncal_stable(void)         { return do_uncal_stable.isStable();}
        // EC
        static float getEC_uncal(void)              { return sens.ec_uncal;}
        static float getEC_uncal_stdev(void)        { return ec_uncal_stable.getDeviasionValue();}
        static float getEC_uncal_stableCount(void)  { return ec_uncal_stable.getStableCount();}
        static bool isEC_uncal_stable(void)         { return ec_uncal_stable.isStable();}
        

    // sub-class 
        class water;
        class air;
};

class Sensor::water{
    friend Sensor;
    private :
        // channel of tentacles
        static uint8_t ch_index;
        // ezo module controller
        static Ezo_uart ezo_Module;
        static bool sens_temp_comp;
        // ezo module info
        static module_info moduleInfo[NUM_OF_EZO];

        // keep the last time of taking data
        static uint32_t lastSensWaterTime;
        static void updateTimeMillis(void);
        static void waterDelay(uint32_t timeDelay);
        // load information of the EZO
        // static void initSensorBoard(void);
        
        // load parameter of reading
        static void loadParamSensor(const char *sens);
        
    public :
        static void setup(void);
        static void app(void);

        static void initSensorBoard(void);
};

class Sensor::air{
    friend Sensor;
    private :
        static uint32_t lastSensTime;
    public :
        static void setup(void);
        static void app(void);
};
#endif