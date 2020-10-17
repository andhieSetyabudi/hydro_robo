#include "Arduino.h"
#include "serial_com.h"
#include "board_io.h"
#include "Sensor.h"
#include "extra\equation.h"

#include "ArduinoJson.h"

#define command_key     F("cmd")
#define calibration_key F("cal")
#define cal_reference   F("sp")
#define cal_currVal     F("cp")

#define EC_sensor_      "ec"
#define pH_sensor_      "ph"
#define DO_sensor_      "do"

void (*resetFunc)(void) = 0;
StaticJsonDocument<500> json_buffer;
const serial_key key_cmd PROGMEM = {

    .ping       = "ping",
    .get_sn     = "get_sn",
    .restart    = "restart",

    .read_pH_uncalibrated = "get_pH_uncal",
    .read_ec_uncalibrated = "get_ec_uncal",
    .read_do_uncalibrated = "get_do_uncal",

    .read_all = "get_all",
    .read_pH = "get_ph",
    .read_conductivity = "get_ec",
    .read_salinity = "get_sal",
    .read_tds = "get_tds",
    .read_specific_of_gravity = "get_sog",
    .read_dissolved_oxygen_mgl = "get_do_mgl",
    .read_dissolved_oxygen_percent = "get_do_%",
    .read_water_temperature = "get_water_temp",
    .read_elevation = "get_elevation",
    .read_air_pressure = "get_air_pressure",
    .read_calibration_file = "get_cal_file",

};

String serial_com::serialBuffer = "";
bool serial_com::serialFlag     = false;
void (*serial_com::halt)(uint32_t t) = NULL;
bool serial_com::reset_by_cmd   = false;
void serialEvent()
{
    serial_com::serialBuffer = "";
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
                // uint16_t lastPos = serial_com::serialBuffer.length();
                // char lastBuffer = serial_com::serialBuffer.charAt(lastPos);
                // if( lastBuffer == '\r' )
                
                serial_com::serialFlag = true;
                break;
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
    serialBuffer.reserve(500);
    reset_by_cmd = false;
}

void serial_com::app()
{
    if (serialFlag )
    {
        // serialBuffer.trim();
        // serialBuffer.toLowerCase();
        Serial.println(serialBuffer);
        Serial.flush();
        parser();
        serialFlag = false;
        serialBuffer = "";
        if (reset_by_cmd)
        {
            reset_by_cmd = false;
            Serial.println(F("Restart module by CMD !"));
            Serial.flush();
            resetFunc();
        }
    }
    if ( Serial.available())
        serialEvent();
}

#define MAX_CAL_REF_NUM     5
void serial_com::parser(void)
{
    StaticJsonDocument<500> parserDoc;
    serialBuffer.toLowerCase();
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
            }
        }
        else
        {
            cmd_ = json_obj[command_key].as<String>();
            parsingByKeyword(cmd_, json_string);
        }
        
    };
    if (json_obj.containsKey(calibration_key) )
    {
        JsonObject sub_obj = json_obj[calibration_key].as<JsonObject>();
        float ref[MAX_CAL_REF_NUM], cur[MAX_CAL_REF_NUM];
        uint8_t len = 0;
        if (!sub_obj.containsKey(cal_reference) && !!sub_obj.containsKey(cal_currVal))
        {
            json_buffer.clear();
            DeserializationError error = deserializeJson(json_buffer, json_string);
            if (error)
                json_buffer.clear();
            json_buffer[calibration_key] = "no reference value";
            serializeJson(json_buffer, json_string);
            json_buffer.clear();
        }
        else
        {
            // load variable for calibration
            // set point as reference
            JsonArray sp_arr = sub_obj[cal_reference].as<JsonArray>();
            len = sp_arr.size() >= MAX_CAL_REF_NUM ? MAX_CAL_REF_NUM : sp_arr.size();
            if (len > 0)
            {
                for ( uint8_t idc = 0; idc < len; idc++)
                {
                    ref[idc] = sub_obj[cal_reference][idc].as<float>();
                }
            }
            else
            {
                ref[0] = sub_obj[cal_reference].as<float>();
            }

            // current reading as value to be setted
            JsonArray cp_arr = sub_obj[cal_currVal].as<JsonArray>();
            len = min ( cp_arr.size(), len );
            if (len > 0)
            {
                for (uint8_t idc = 0; idc < len; idc++)
                {
                    cur[idc] = sub_obj[cal_currVal][idc].as<float>();
                }
            }
            else
            {
                cur[0] = sub_obj[cal_currVal].as<float>();
            }
            //============================================================

            if (sub_obj.containsKey(F("type")))
            {
                String sens_type = sub_obj[F("type")].as<String>();
                sens_type.trim();
                // sens_type.toLowerCase();
                Serial.println("kalibrasi : " + sens_type);
                if ( sens_type.equalsIgnoreCase(boardKey[0]) ) // pH
                {
                    if ( len > 1)
                        RegresionLinear(cur, ref, len, &deviceParameter.pH_calibration_parameter.slope, &deviceParameter.pH_calibration_parameter.offset);
                    else
                    {
                        deviceParameter.pH_calibration_parameter.slope = ref[len] / cur [len]; 
                        deviceParameter.pH_calibration_parameter.offset = 0;
                    }
                    if (isnan(deviceParameter.pH_calibration_parameter.slope)) deviceParameter.pH_calibration_parameter.slope = 1;
                    if (isnan(deviceParameter.pH_calibration_parameter.offset)) deviceParameter.pH_calibration_parameter.offset = 0;
                    Serial.println(" pH slope : " + String(deviceParameter.pH_calibration_parameter.slope) + "\toffset" + String(deviceParameter.pH_calibration_parameter.offset));
                    backUpMemory();
                }
                else if (sens_type.equalsIgnoreCase(boardKey[1])) // DO
                {
                    if (len > 1)
                        RegresionLinear(cur, ref, len, &deviceParameter.DO_calibration_parameter.slope, &deviceParameter.DO_calibration_parameter.offset);
                    else
                    {
                        deviceParameter.DO_calibration_parameter.slope = ref[len] / cur[len];
                        deviceParameter.DO_calibration_parameter.offset = 0;
                    }
                    if (isnan(deviceParameter.DO_calibration_parameter.slope))
                        deviceParameter.DO_calibration_parameter.slope = 1;
                    if (isnan(deviceParameter.DO_calibration_parameter.offset))
                        deviceParameter.DO_calibration_parameter.offset = 0;
                    Serial.println(" DO slope : " + String(deviceParameter.DO_calibration_parameter.slope) + "\toffset" + String(deviceParameter.DO_calibration_parameter.offset));
                    backUpMemory();
                }
                else if (sens_type.equalsIgnoreCase(boardKey[2])) // EC
                {
                    if (len > 1)
                        RegresionLinear(cur, ref, len, &deviceParameter.EC_calibration_parameter.slope, &deviceParameter.EC_calibration_parameter.offset);
                    else
                    {
                        deviceParameter.EC_calibration_parameter.slope = ref[len] / cur[len];
                        deviceParameter.EC_calibration_parameter.offset = 0;
                    }
                    if (isnan(deviceParameter.EC_calibration_parameter.slope))
                        deviceParameter.EC_calibration_parameter.slope = 1;
                    if (isnan(deviceParameter.EC_calibration_parameter.offset))
                        deviceParameter.EC_calibration_parameter.offset = 0;
                    Serial.println(" EC slope : " + String(deviceParameter.EC_calibration_parameter.slope) + "\toffset" + String(deviceParameter.EC_calibration_parameter.offset));
                    backUpMemory();
                }
                else
                {
                    json_buffer.clear();
                    DeserializationError error = deserializeJson(json_buffer, json_string);
                    if (error)
                        json_buffer.clear();
                    json_buffer[calibration_key] = F("unknown sensor");
                    serializeJson(json_buffer, json_string);
                    json_buffer.clear();
                }
            }
        };
    }
    Serial.println(json_string);
    Serial.flush();
}


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
    if (plain.equalsIgnoreCase((const char *)pgm_read_word(&(key_cmd.read_pH))) || read_all_param ) {
        json_buffer.createNestedObject(F("pH"));
        json_buffer[F("pH")]["value"] = Sensor::getpH();
        json_buffer[F("pH")][F("stdev")] = Sensor::getPH_stdev();
        json_buffer[F("pH")][F("stable")] = Sensor::isPHStable();
    }

    if (plain.equalsIgnoreCase((const char *)pgm_read_word(&(key_cmd.read_conductivity))) || read_all_param) {
        json_buffer.createNestedObject(F("conductivity"));
        json_buffer[F("conductivity")][F("value")] = Sensor::getConductivity();
        json_buffer[F("conductivity")][F("stdev")] = Sensor::getEC_stdev();
        json_buffer[F("conductivity")][F("stable")] = Sensor::isConductivityStable();
    }

    if (plain.equalsIgnoreCase((const char *)pgm_read_word(&(key_cmd.read_salinity))) || read_all_param)
        json_buffer[F("salinity")] = Sensor::getSalinity();
    
    if (plain.equalsIgnoreCase((const char *)pgm_read_word(&(key_cmd.read_specific_of_gravity))) || read_all_param)
        json_buffer[F("SoG")] = Sensor::getSpecifivGravity();
    
    if (plain.equalsIgnoreCase((const char *)pgm_read_word(&(key_cmd.read_tds))) || read_all_param)
        json_buffer[F("TDS")] = Sensor::getTDS();
    
    if (plain.equalsIgnoreCase((const char *)pgm_read_word(&(key_cmd.read_dissolved_oxygen_percent))) || read_all_param)
        json_buffer[F("DO_%")] = Sensor::getDO_percent();
    
    if (plain.equalsIgnoreCase((const char *)pgm_read_word(&(key_cmd.read_dissolved_oxygen_mgl))) || read_all_param){
        json_buffer.createNestedObject(F("DO_mgl"));
        json_buffer[F("DO_mgl")][F("value")] = Sensor::getDO_mgl();
        json_buffer[F("DO_mgl")][F("stdev")] = Sensor::getDO_stdev();
        json_buffer[F("DO_mgl")][F("stable")] = Sensor::isDOStable();
    }

    if (plain.equalsIgnoreCase((const char *)pgm_read_word(&(key_cmd.read_water_temperature))) || read_all_param)
        json_buffer[F("Water Temp")] = Sensor::getWaterTemperature();

    if ( plain.equalsIgnoreCase((const char *)pgm_read_word(&(key_cmd.read_elevation))) )
        json_buffer[F("elevation")] = deviceParameter.elevation;

    if (plain.equalsIgnoreCase((const char *)pgm_read_word(&(key_cmd.read_calibration_file))))
    {
        json_buffer.createNestedObject(F("Cal"));
        json_buffer[F("Cal")].createNestedObject(F("pH"));
        json_buffer[F("Cal")][F("pH")][F("gain")] = deviceParameter.pH_calibration_parameter.slope;
        json_buffer[F("Cal")][F("pH")][F("offset")] = deviceParameter.pH_calibration_parameter.offset;

        json_buffer[F("Cal")].createNestedObject(F("DO"));
        json_buffer[F("Cal")][F("DO")][F("gain")] = deviceParameter.DO_calibration_parameter.slope;
        json_buffer[F("Cal")][F("DO")][F("offset")] = deviceParameter.DO_calibration_parameter.offset;

        json_buffer[F("Cal")].createNestedObject(F("EC"));
        json_buffer[F("Cal")][F("EC")][F("gain")] = deviceParameter.EC_calibration_parameter.slope;
        json_buffer[F("Cal")][F("EC")][F("offset")] = deviceParameter.EC_calibration_parameter.offset;
    }

    if (plain.equalsIgnoreCase((const char *)pgm_read_word(&(key_cmd.get_sn))))
    {
        String buf_sn="";
        for (uint8_t l = 0; l < UniqueIDsize; l++){
            buf_sn += String(serialDevice[l],HEX);
        }
        json_buffer[F("SN")] = buf_sn;
    }

    else if (plain.equalsIgnoreCase((const char *)pgm_read_word(&(key_cmd.restart))))
        reset_by_cmd = true;

    else if (plain.equalsIgnoreCase((const char *)pgm_read_word(&(key_cmd.ping))))
        json_buffer[F("response")] = F("ok");
    else
    {
        if ( json_buffer.isNull() )
            json_buffer[F("response")] = plain + F(" unknown command");
    }
    json_str = "";
    serializeJson(json_buffer, json_str);
    json_buffer.clear();
}