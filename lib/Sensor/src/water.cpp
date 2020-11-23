#include "board_io.h"
#include "Sensor.h"
#include "extra/equation.h"



SoftwareSerial ezoSerial(EZO_RX_PIN, EZO_TX_PIN);
    // initialize variable, system requirement. Sometime error occur by compiler
uint32_t Sensor::water::lastSensWaterTime = 0;
uint8_t Sensor::water::ch_index = 0;
bool Sensor::water::sens_temp_comp=false;
Ezo_uart Sensor::water::ezo_Module(ezoSerial);
// ezo module info
module_info Sensor::water::moduleInfo[NUM_OF_EZO];

void Sensor::water::updateTimeMillis(void)
{
    if (Sensor::getTick == NULL)
        lastSensWaterTime = millis();
    else
        lastSensWaterTime = Sensor::getTick();
}

void Sensor::water::waterDelay(uint32_t timeDelay)
{
    if ( Sensor::halt == NULL )
        delay(timeDelay);
    else
        Sensor::halt(timeDelay);
}

void Sensor::water::initSensorBoard(void)
{
    ezoSerial.begin(9600);
    ezoSerial.flush();
    uint8_t infoIndex = 0;
    Serial.println(F("{\"status\":\"Water Sensor Initializing\"}"));
    for (uint8_t id = 0; id < TENTACLES_CH_NUM; id++)
    {
        tentacles_open_channel(id);
        waterDelay(20);
        char buf_msg[20] = "";
        ezo_Module.flush_rx_buffer();
        ezo_Module.send_cmd("i", buf_msg, 19);
        if (strlen(buf_msg) > 1) {
            //Serial.println("info = " + String(buf_msg));
            for (uint8_t key = 0; key < NUM_OF_EZO; key++)
            {
              //  Serial.println("key = " + String(boardKey[key]));
                if (strstr((const char*)buf_msg, boardKey[key]) != NULL)
                {
                    strcpy(moduleInfo[infoIndex].name, boardKey[key]);
                    moduleInfo[infoIndex].ch = id;
                    moduleInfo[infoIndex].status = 0;
                    infoIndex++;
                    Serial.println("{\"status\":\"module found " + String(boardKey[key]) + " at ch" + String(id)+"\"}");
                    ezo_Module.send_cmd("C,0", NULL, 0); //send the command to turn off continuous mode
                                                         //in this case we arent concerned about waiting for the reply
                    waterDelay(100);
                    ezo_Module.send_cmd("*OK,0", NULL, 0);  // turn of "OK" reply
                    if ( key == 2 ) // specialy case for EC
                    {
                        ezo_Module.send_cmd("K,1", NULL, 0);
                        waterDelay(10);
                        ezo_Module.send_cmd(F("O,EC,1"), NULL, 0);
                        waterDelay(10);
                        ezo_Module.send_cmd(F("O,TDS,0"), NULL, 0);
                        waterDelay(10);
                        ezo_Module.send_cmd(F("O,S,0"), NULL, 0);
                        waterDelay(10);
                        ezo_Module.send_cmd(F("O,SG,0"), NULL, 0);
                    }
                    else if ( key == 1 ) // specialy case for DO
                    {
                        // turn off mgl value, only take percent value
                        ezo_Module.send_cmd("O,mg,0", NULL, 0);
                        waterDelay(10);
                        ezo_Module.send_cmd("O,%,1", NULL, 0);
                    }
                    // clear calibration data except RTD module
                    if (key < ( NUM_OF_EZO - 1 )) // NUM_OF_EZO - 1 meaning last index is RTD sensor
                    {
                        sprintf_P(buf_msg, (const char *)F(""));
                        if ( ezo_Module.send_cmd("Cal,?", buf_msg, 19) ){
                            if (strcmp(buf_msg, "*ER") != 0)    // no response error
                            {
                                // Serial.println("response calibration: "+String(buf_msg));
                                *buf_msg += strlen_P((const char*)F("?Cal,"));
                                if ( atoi(buf_msg) != 0) // need to reset cal
                                    ezo_Module.send_cmd("Cal,clear", NULL, 0);
                                // Serial.println("calibration status : "+String(atoi(buf_msg)));
                            };
                        };
                    }
                    break; // quit from loop
                };
            };
        }
        if (infoIndex >= NUM_OF_EZO)
            break;
        else if (id == TENTACLES_CH_NUM - 1)
            Serial.println(F("{\"status\":\"Module Not Found !\"}"));
        waterDelay(20);
    };
}

void Sensor::water::loadParamSensor(const char *sens)
{
    float floating_buffer = 0;
    ezo_Module.flush_rx_buffer();
    if( !ezo_Module.send_read() )
        return;
    if (strstr((const char *)sens, boardKey[0]) != NULL) {    // pH
        floating_buffer = ezo_Module.get_reading();
        // pH is compensated by temperature
        float compenFactor = 0;
        float tmp_temp = (getWaterTemperature() > 60.0f) ? 60.0f : getWaterTemperature();
        compenFactor = ((25.f + 273.15f) / (tmp_temp + 273.15f));
        compenFactor = isnan(compenFactor) ? 1 : (compenFactor < 0) ? 0.001 : compenFactor;
        Sensor::sens.pH_uncal = 7.01f + ((floating_buffer - 7.01f) * compenFactor);
        Sensor::pH_uncal_stable.pushToBuffer(getPH_uncal());
        // limitation of value ( range 0 - 14 ) => normalize
        Sensor::sens.pH_uncal = (Sensor::sens.pH_uncal < 0.f) ? 0.00f : (Sensor::sens.pH_uncal > 14.0f) ? 14.0f : Sensor::sens.pH_uncal;

        Sensor::sens.pH = deviceParameter.pH_calibration_parameter.slope * getPH_uncal() + 
                            deviceParameter.pH_calibration_parameter.offset;
        // limitation of value ( range 0 - 14 ) => normalize
        Sensor::sens.pH = (Sensor::sens.pH < 0.f) ? 0.00f : (Sensor::sens.pH > 14.0f) ? 14.0f : Sensor::sens.pH;
        Sensor::pH_stable_.pushToBuffer(getpH()); // update stability detector data series
    }
    else if (strstr((const char *)sens, boardKey[1]) != NULL) { // DO
        Sensor::sens.do_percent_uncal = ezo_Module.get_reading();
        // push to stabilitydetector buffer
        Sensor::do_uncal_stable.pushToBuffer(getDO_percent_uncal());

        Sensor::sens.DO2_percent = deviceParameter.DO_calibration_parameter.slope * getDO_percent_uncal() + 
                                    deviceParameter.DO_calibration_parameter.offset;
        // normalize
        Sensor::sens.DO2_percent = Sensor::sens.DO2_percent < 0 ? 0 : Sensor::sens.DO2_percent;
        floating_buffer = saturationDOvalue(getWaterTemperature(), getAirPressure(), getConductivity());
        Sensor::sens.DO2_mgl = floating_buffer * getDO_percent();
        Sensor::DO_stable_.pushToBuffer(getDO_mgl()); // update stability detector data series
    }
    else if (strstr((const char *)sens, boardKey[2]) != NULL) { // EC
        floating_buffer = ezo_Module.get_reading();
        
        float temp_compen = getWaterTemperature() > 60 ? 60 : (getWaterTemperature() < 0) ? 0 : getWaterTemperature();
        Sensor::sens.ec_uncal       = conductivityTempCompensation(floating_buffer, temp_compen);
        Sensor::ec_uncal_stable.pushToBuffer(getEC_uncal());
        
        Sensor::sens.conductivity = deviceParameter.EC_calibration_parameter.slope * getEC_uncal() +
                                    deviceParameter.EC_calibration_parameter.offset;
        //normalize
        Sensor::sens.conductivity = Sensor::sens.conductivity < 0 ? 0 : Sensor::sens.conductivity > 80000 ? 80000 :Sensor ::sens.conductivity;

        // Sensor::sens.conductivity   = conductivityTempCompensation(floating_buffer, temp_compen);
        Sensor::sens.salinity = condToSal(getConductivity(), getWaterTemperature());

        Sensor::sens.specificOfGravity  = density_salt_water(getSalinity(), getWaterTemperature(), getAirPressure()*1000.f);
        Sensor::sens.specificOfGravity /= density_salt_water(0.00f, getWaterTemperature(), getAirPressure()*1000.f);

        Sensor::EC_stable_.pushToBuffer(getConductivity());       // update stability detector data series
    }
    else if (strstr((const char *)sens, boardKey[3]) != NULL) { // RTD
        Sensor::sens.water_temperature = ezo_Module.get_reading();
        Sensor::sens.airPressure_in_kpa = getPressureFrom(Sensor::sens.water_temperature, deviceParameter.elevation);

        Sensor::water_temp_stable_.pushToBuffer(getWaterTemperature()); // update stability detector data series
    }
}

void Sensor::water::setup(void)
{
    // waiting for system ready
    while( Sensor::isSleep() ) waterDelay(20);

    initSensorBoard();
    // taking first tick and store it
    updateTimeMillis();
}

void Sensor::water::app(void)
{
    // if sensor need to sleep
    if ( Sensor::isSleep() ) return;

    if (ch_index >= NUM_OF_EZO ){
        sens_temp_comp = ~sens_temp_comp;
        ch_index = 0;
    }
    tentacles_open_channel(moduleInfo[ch_index].ch);
    waterDelay(10);
    loadParamSensor(moduleInfo[ch_index].name);
    
    // // set temperature compensation
    // if ( sens_temp_comp ){
    //     waterDelay(100);
    //     if ( sens.water_temperature >= 0 && sens.water_temperature <= 75 )
    //         ezo_Module.send_cmd("T,"+String(sens.water_temperature,2),NULL, 0);
    //     else
    //         ezo_Module.send_cmd("T,25.00", NULL, 0);
    // };
    // Sensor::waterParamInfo();
    ch_index++;
}