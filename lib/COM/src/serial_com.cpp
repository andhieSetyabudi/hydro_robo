#include "Arduino.h"
#include "serial_com.h"
#include "board_io.h"
#include "Sensor.h"

#include "ArduinoJson.h"

#define command_key F("cmd")

const serial_key key_cmd PROGMEM = {
    .ping               = "ping",
    .get_sn             = "get_sn",
    .read_all           = "get_all",
    .read_pH            = "get_ph",
    .read_conductivity  = "get_ec",
    .read_salinity      = "get_sal",
    .read_tds           = "get_tds",
    .read_specific_of_gravity = "get_sog",
    .read_dissolved_oxygen_mgl = "get_DO_mgl",
    .read_dissolved_oxygen_percent = "get_DO_%",
    .read_water_temperature        = "get_water_temp",
    .read_calibration_file         = "get_cal_file",


};


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
        String tmp;
        serializeJson(error_info, Serial);
        Serial.print(F("\r\n"));
        return;
    }
    JsonObject json_obj = parserDoc.as<JsonObject>();
    String json_string = " ";
    if (json_obj.containsKey(command_key)) // check for user cmd request
    {
        JsonArray arr = json_obj[command_key].as<JsonArray>();
        int arrSize = arr.size();
        Serial.println("found key : " + String(arrSize));

        String cmd_ = "";
        cmd_.reserve(15);
        if( arrSize > 0 ){
            for (uint8_t idx = 0; idx < arrSize; idx++)
            {
                cmd_ = "";
                cmd_ = json_obj[command_key][idx].as<String>();
                parsingByKeyword(cmd_, json_string);
                Serial.println(cmd_);
                halt(10);
            }
        }
        else
        {
            cmd_ = json_obj[command_key].as<String>();
            parsingByKeyword(cmd_, json_string);
        }
        Serial.println(json_string);
    }
}

StaticJsonDocument<500> json_buffer;
void serial_com::parsingByKeyword(const String &plain, String &json_str)
{
    json_buffer.clear();
    if ( json_str.length() > 3 )
    {
        DeserializationError error = deserializeJson(json_buffer, json_str);
        if (error)
        {
            json_buffer.clear();
            
        }
    }
    
    // for parameter sensor_reading
    bool read_all_param = plain.equals((const char *)pgm_read_word(&(key_cmd.read_all)));
    if (plain.equals((const char *)pgm_read_word(&(key_cmd.read_pH))) || read_all_param )
        json_buffer[F("pH")] = Sensor::getpH();
    if (plain.equals((const char *)pgm_read_word(&(key_cmd.read_conductivity))) || read_all_param)
        json_buffer[F("conductivity")] = Sensor::getConductivity();
    if (plain.equals((const char *)pgm_read_word(&(key_cmd.read_salinity))) || read_all_param)
        json_buffer[F("salinity")] = Sensor::getSalinity();
    if (plain.equals((const char *)pgm_read_word(&(key_cmd.read_specific_of_gravity))) || read_all_param)
        json_buffer[F("SoG")] = 1;
    if (plain.equals((const char *)pgm_read_word(&(key_cmd.read_tds))) || read_all_param)
        json_buffer[F("TDS")] = 1;
    if (plain.equals((const char *)pgm_read_word(&(key_cmd.read_dissolved_oxygen_percent))) || read_all_param)
        json_buffer[F("DO_%")] = 10.0;
    if (plain.equals((const char *)pgm_read_word(&(key_cmd.read_dissolved_oxygen_mgl))) || read_all_param)
        json_buffer[F("DO_mgl")] = 10.0;
    if (plain.equals((const char *)pgm_read_word(&(key_cmd.read_water_temperature))) || read_all_param)
        json_buffer[F("Water Temp")] = Sensor::getWaterTemperature();

    if (plain.equals((const char *)pgm_read_word(&(key_cmd.get_sn))))
        json_buffer[F("SN")] = serialDevice;
    if (plain.equals((const char *)pgm_read_word(&(key_cmd.ping))))
        json_buffer[F("response")] = F("ok");
    else
        json_buffer[F("response")] = plain + F(" unknown command");
    json_str = "";
    serializeJson(json_buffer, json_str);
    json_buffer.clear();
}