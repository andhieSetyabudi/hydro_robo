#include "Arduino.h"
#include "serial_com.h"
#include "board_io.h"
#include "Sensor.h"

#include "ArduinoJson.h"


// const serial_keyword serial_key_ = (serial_keyword){
//     .read_all = "read-all",
// };

String serial_com::serialBuffer = "";
bool serial_com::serialFlag     = false;
void (*serial_com::halt)(uint32_t t) = NULL;

void serialEvent()
{
    while ( Serial.available() )
    {
        // get the new byte:
        char inChar = (char)Serial.read();
        // add it to the inputString:
        serial_com::serialBuffer += inChar;
        // if the incoming character is a newline, set a flag so the main loop can
        // do something about it:
        if ( inChar == '\n' )
        {
            // take the last string value
            uint16_t lastPos = serial_com::serialBuffer.length();
            char lastBuffer = serial_com::serialBuffer.charAt(lastPos);
            // if( lastBuffer == '\r' )
                serial_com::serialFlag = true;
        }
    }
}


void serial_com::setHalt(void(*halt)(uint32_t t))
{
    serial_com::halt = halt;
}

void serial_com::serial_halt(uint32_t t)
{
    if (halt == NULL)
        delay(t);
    else
        halt(t);
}

void serial_com::setup()
{
    Serial.begin(SERIAL_BAUD);
    if (!Serial){
        serial_halt(1000);
    }
    serialBuffer.reserve(256);
}

void serial_com::app()
{
    if (serialFlag )
    {
        serialBuffer.trim();
        serialBuffer.toLowerCase();
        parser();
        Serial.println(serialBuffer);
        serialFlag = false;
        serialBuffer = "";
    }
}

void serial_com::parser(void)
{
    StaticJsonDocument<368> parserDoc;
    DeserializationError error = deserializeJson(parserDoc, serialBuffer);
    if (error ){
        StaticJsonDocument<80> error_info;
        error_info["response"] = "error";
        error_info["status"]= "unknown or corrupt ";
        serializeJsonPretty(error_info, Serial);
        Serial.print("\r\n");
        return;
    }
    JsonObject json_obj = parserDoc.as<JsonObject>();
    if (json_obj["command"].as<String>().length() > 0){
        String command = json_obj["command"].as<String>();
        command.trim();
        if ( command.equals("ping")) {
            Serial.print("{\"response\":\"OK\"\r\n");
        }
        else if ( command.equals("read-all")) {
            StaticJsonDocument<300> sensor_data;
            sensor_data["temperature"] = Sensor::getWaterTemperature();
            sensor_data["pH"] = Sensor::getpH();
            sensor_data["conductivity"] = Sensor::getConductivity();
            sensor_data["salinity"] = Sensor::getpH();
            sensor_data["TDS"] = Sensor::getpH();
            sensor_data["SoG"] = Sensor::getpH();
            sensor_data["DO2_mgl"] = Sensor::getpH();
            sensor_data["DO2_percent"] = Sensor::getpH();

            serializeJsonPretty(sensor_data, Serial);
            Serial.print("\r\n");
        }
    }
        Serial.println(json_obj["command"].as<String>());
}


