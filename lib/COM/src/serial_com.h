// #pragma once
#ifndef _SERIAL_COM_H_
#define _SERIAL_COM_H

#include "Sensor.h"

struct serial_keyword{
    const char* ping = "ping";
// read command
    const char* read_all = "read-all";
    const char* read_pH;
    const char* read_conductivity;
    const char* read_salinity;
    const char* read_tds;
    const char* read_specific_of_gravity;
    const char* read_dissolved_oxygen_mgl;
    const char* read_dissolved_oxygen_percent;
    const char* read_water_temperature;
    const char* read_calibration_file;

// write command
    const char* write_calibration_file;
    const char* write_tds_constant;
    const char* write_elevation;
};

class serial_com{
    private:
        static void (*halt)(uint32_t t);
        static void serial_halt(uint32_t t);
        
    public:
        static void setup();
        static void app();
        static void parser(void);

        static void setHalt(void (*halt) (uint32_t t));

        static String serialBuffer;
        static bool serialFlag;
};
#endif