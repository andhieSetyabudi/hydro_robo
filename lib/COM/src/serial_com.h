#pragma once
#ifndef _SERIAL_COM_H_
#define _SERIAL_COM_H

#include "Firmata.h"
#include "Sensor.h"

struct {
    const char* ping;
// read command
    const char* read_all;
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

}serial_keyword;


class serial_com{
private:

public:
    static void setuo();
    static void app();
};
#endif