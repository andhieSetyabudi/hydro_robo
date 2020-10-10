
#ifndef _SENSOR_H_
#define _SENSOR_H_

#include "Arduino.h"
#include "SoftwareSerial.h"
#include "stdio.h"
#include "string.h"
#include "math.h"

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
            DO2_percent;
};

class Sensor{
    private : 
        static bool systemSleep;

        static Sens_val sens;
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

        static float getWaterTemperature(void)  { return sens.water_temperature;}
        static float getpH(void)                { return sens.pH;}
        static float getConductivity(void)      { return sens.conductivity;}
        static float getSalinity(void)          { return sens.salinity;}
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