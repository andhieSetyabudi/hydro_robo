#include "board_io.h"
#include "Sensor.h"
#include "extra/equation.h"

const char boardKey[][4] = {"pH", "DO", "EC", "RTD"};

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
    Serial.println("Water Sensor Initializing");
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
                    Serial.println("module found " + String(boardKey[key])+" at ch"+String(id));
                    ezo_Module.send_cmd("C,0", NULL, 0); //send the command to turn off continuous mode
                                                         //in this case we arent concerned about waiting for the reply
                    waterDelay(100);
                    ezo_Module.send_cmd("*OK,0", NULL, 0);
                    if ( key == 2 ) // specialy case for EC
                    {
                        ezo_Module.send_cmd("K,1", NULL, 0);
                    }
                    else if ( key == 1 ) // specialy case for DO
                    {
                        ezo_Module.send_cmd("O,mg,0", NULL, 0);
                        waterDelay(10);
                        ezo_Module.send_cmd("O,%,1", NULL, 0);
                    }
                    break; // quit from loop
                };
            };
        }
        if (infoIndex >= NUM_OF_EZO)
            break;
        waterDelay(20);
    };
}

void Sensor::water::loadParamSensor(const char *sens)
{
    ezo_Module.flush_rx_buffer();
    if( !ezo_Module.send_read() )
        return;
    if (strstr((const char *)sens, boardKey[0]) != NULL) {    // pH
        Sensor::sens.pH = ezo_Module.get_reading();
    }
    else if (strstr((const char *)sens, boardKey[1]) != NULL) { // DO
        Sensor::sens.DO2_percent = ezo_Module.get_reading();
    }
    else if (strstr((const char *)sens, boardKey[2]) != NULL) { // EC
        Sensor::sens.conductivity = ezo_Module.get_reading();
        Sensor::sens.salinity = condToSal(Sensor::sens.conductivity, Sensor::sens.water_temperature);
    }
    else if (strstr((const char *)sens, boardKey[3]) != NULL) { // RTD
        Sensor::sens.water_temperature = ezo_Module.get_reading();
    }
}

void Sensor::water::setup(void)
{
    sens = {0};
    // waiting for system ready
    while( Sensor::isSleep() ) waterDelay(20);

    // taking first tick and store it
    updateTimeMillis();
}

void Sensor::water::app(void)
{
    // if sensor need to sleep
    // if ( Sensor::isSleep() ) return;

    if (ch_index >= NUM_OF_EZO ){
        sens_temp_comp = ~sens_temp_comp;
        ch_index = 0;
    }
    tentacles_open_channel(moduleInfo[ch_index].ch);
    waterDelay(100);
    loadParamSensor(moduleInfo[ch_index].name);
    
    // set temperature compensation
    if ( sens_temp_comp ){
        waterDelay(100);
        if ( sens.water_temperature >= 0 && sens.water_temperature <= 75 )
            ezo_Module.send_cmd("T,"+String(sens.water_temperature,2),NULL, 0);
        else
            ezo_Module.send_cmd("T,25.00", NULL, 0);
    };
    Sensor::waterParamInfo();
    ch_index++;
}