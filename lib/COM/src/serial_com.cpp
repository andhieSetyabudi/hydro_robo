#include "Arduino.h"
#include "serial_com.h"
#include "board_io.h"
#include "Sensor.h"
#include "extra\equation.h"

#include "ArduinoJson.h"

#define buffer_serial_len       512

#define command_key     F("cmd")
#define calibration_key F("cal")
#define set_key         F("set")
#define cal_reference   F("sp")
#define cal_currVal     F("cp")

#define EC_sensor_      "ec"
#define pH_sensor_      "ph"
#define DO_sensor_      "do"


void (*resetFunc)(void) = 0;
StaticJsonDocument<buffer_serial_len> json_buffer;
String json_string = " ";
const serial_key key_cmd PROGMEM = {

    .ping = "ping",
    .get_sn = "get_sn",
    .restart = "restart",

    .read_pH_uncalibrated = "get_pH_uncal",
    .read_ec_uncalibrated = "get_ec_uncal",
    .read_do_uncalibrated = "get_do_uncal",

    .read_all                       = "get_all",
    .read_pH                        = "get_ph",
    .read_conductivity              = "get_ec",
    .read_salinity                  = "get_sal",
    .read_tds                       = "get_tds",
    .read_specific_of_gravity       = "get_sog",
    .read_dissolved_oxygen_mgl      = "get_do_mgl",
    .read_dissolved_oxygen_percent  = "get_do_%",
    .read_water_temperature         = "get_water_temp",
    .read_elevation                 = "get_elevation",
    .read_air_pressure              = "get_air_pressure",
    .read_calibration_file          = "get_cal_file",
    .reset_cal_file                 = "rst_cal_file",

    .write_calibration_file         = "cal_file",
    .write_tds_constant             = "tds_const",
    .set_elevation                  = "elevation",
    .setPrecision_file              = "precision_file",
};

String serial_com::serialBuffer = "";
bool serial_com::serialFlag     = false;
void (*serial_com::halt)(uint32_t t) = NULL;
bool serial_com::reset_by_cmd   = false;
void serialEvent_()
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
            serial_com::serialFlag = true;
            // break;
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
    serialBuffer.reserve(200);
    reset_by_cmd = false;
}

void serial_com::app()
{
    if (serialFlag )
    {
        Serial.flush();
        //Serial.println(serialBuffer);
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
        serialEvent_();
}

#define MAX_CAL_REF_NUM     5
void serial_com::parser(void)
{
    StaticJsonDocument<buffer_serial_len> parserDoc;
    serialBuffer.trim();
    serialBuffer.toLowerCase();
    DeserializationError error = deserializeJson(parserDoc, serialBuffer);
    if (error ){
        StaticJsonDocument<100> error_info;
        error_info["response"] = "error";
        error_info["status"]= "unknown or corrupt ";
        String tmp;
        serializeJson(error_info, Serial);
        Serial.print(F("\r\n"));
        return;
    }
    JsonObject json_obj = parserDoc.as<JsonObject>();
    
    if (json_obj.containsKey(command_key)) // check for user cmd request
    {
        JsonArray arr = json_obj[command_key].as<JsonArray>();
        int arrSize = arr.size();
        // Serial.println("found key : " + String(arrSize));

        String cmd_ = "";
        cmd_.reserve(15);
        if( arrSize > 0 ){
            for (uint8_t idx = 0; idx < arrSize; idx++)
            {
                cmd_ = "";
                cmd_ = json_obj[command_key][idx].as<String>();
                parsingByKeyword(cmd_, json_string);
                // Serial.println(cmd_);
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
        if ( !sub_obj.containsKey(cal_reference) || !sub_obj.containsKey(cal_currVal) ) 
        { 
            json_buffer.clear();
            DeserializationError error = deserializeJson(json_buffer, json_string);
            if (error)
                json_buffer.clear();
            json_buffer[calibration_key] = F("no reference value");
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
                // Serial.println("kalibrasi : " + sens_type);
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

                    // save to EEPROM
                    if (backUpMemory())
                        calibration_done(json_string, (const char *)"pH", (const char *)"Success");
                    else
                        calibration_done(json_string, (const char *)"pH", (const char *)"Failed");
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
                    if ( backUpMemory() )
                        calibration_done(json_string, (const char *)"DO", (const char *)"Success");
                    else
                        calibration_done(json_string, (const char *)"DO", (const char *)"Failed");
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
                    // Serial.println(" EC slope : " + String(deviceParameter.EC_calibration_parameter.slope) + "\toffset" + String(deviceParameter.EC_calibration_parameter.offset));
                    
                    // save to eeprom
                    if (backUpMemory())
                        calibration_done(json_string, (const char *)"EC", (const char *)"Success");
                    else
                        calibration_done(json_string, (const char *)"EC", (const char *)"Failed");
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
    };
    if (json_obj.containsKey(set_key))
    {
        JsonArray arr = json_obj[command_key].as<JsonArray>();
        int arrSize = arr.size();
        // Serial.println("found key : " + String(arrSize));

        String cmd_ = "";
        cmd_.reserve(15);
        if (arrSize > 0)
        {
            for (uint8_t idx = 0; idx < arrSize; idx++)
            {
                cmd_ = "";
                cmd_ = json_obj[command_key][idx].as<String>();
                parsingToSetParam(cmd_, json_string);
            }
        }
        else
        {
            cmd_ = json_obj[command_key].as<String>();
            parsingToSetParam(cmd_, json_string);
        }
    };

    Serial.println(json_string);
    Serial.flush();
    json_string = "";
}


void serial_com::parsingByKeyword(const String &plain, String &json_str)
{
    StaticJsonDocument<buffer_serial_len> json_buffer_msg;
    // json_buffer_msg.clear();
    if ( json_str.length() > 3 )
    {
        DeserializationError error = deserializeJson(json_buffer_msg, json_str);
        if (error)
        {
            json_buffer_msg.clear();
        }
    }
    

    // for taking uncalibrated sensor value
    // pH uncalibrated
    if ( plain.equalsIgnoreCase((const char *)pgm_read_word(&(key_cmd.read_pH_uncalibrated))) )
    {
        json_buffer_msg.createNestedObject(F("pH_uncal"));
        json_buffer_msg[F("pH_uncal")]["value"] = Sensor::getPH_uncal();
        json_buffer_msg[F("pH_uncal")][F("stdev")] = Sensor::getPH_uncal_stdev();
        json_buffer_msg[F("pH_uncal")][F("stable_count")] = Sensor::getPH_uncal_stableCount();
        json_buffer_msg[F("pH_uncal")][F("stable")] = Sensor::isPH_uncal_stable();
    }

    // conductivity uncalibrated
    if (plain.equalsIgnoreCase((const char *)pgm_read_word(&(key_cmd.read_ec_uncalibrated))))
    {
        json_buffer_msg.createNestedObject(F("EC_uncal"));
        json_buffer_msg[F("EC_uncal")]["value"] = Sensor::getEC_uncal();
        json_buffer_msg[F("EC_uncal")][F("stdev")] = Sensor::getEC_uncal_stdev();
        json_buffer_msg[F("EC_uncal")][F("stable_count")] = Sensor::getEC_uncal_stableCount();
        json_buffer_msg[F("EC_uncal")][F("stable")] = Sensor::isEC_uncal_stable();
    }

    // conductivity uncalibrated
    if (plain.equalsIgnoreCase((const char *)pgm_read_word(&(key_cmd.read_do_uncalibrated))))
    {
        json_buffer_msg.createNestedObject(F("DO_uncal"));
        json_buffer_msg[F("DO_uncal")]["value"] = Sensor::getDO_percent_uncal();
        json_buffer_msg[F("DO_uncal")][F("stdev")] = Sensor::getDO_uncal_stdev();
        json_buffer_msg[F("DO_uncal")][F("stable_count")] = Sensor::getDO_uncal_stableCount();
        json_buffer_msg[F("DO_uncal")][F("stable")] = Sensor::isDO_uncal_stable();
    }

    // for parameter sensor_reading
    bool read_all_param = plain.equals((const char *)pgm_read_word(&(key_cmd.read_all)));

    // pH
    if (plain.equalsIgnoreCase((const char *)pgm_read_word(&(key_cmd.read_pH))) || read_all_param ) {
        json_buffer_msg.createNestedObject(F("pH"));
        json_buffer_msg[F("pH")]["value"] = Sensor::getpH();
        json_buffer_msg[F("pH")][F("stdev")] = Sensor::getPH_stdev();
        json_buffer_msg[F("pH")][F("stable")] = Sensor::isPHStable();
    }

    // conductivity
    if (plain.equalsIgnoreCase((const char *)pgm_read_word(&(key_cmd.read_conductivity))) || read_all_param) {
        json_buffer_msg.createNestedObject(F("EC"));
        json_buffer_msg[F("EC")][F("value")] = Sensor::getConductivity();
        json_buffer_msg[F("EC")][F("stdev")] = Sensor::getEC_stdev();
        json_buffer_msg[F("EC")][F("stable")] = Sensor::isConductivityStable();
    }

    // salinity
    if (plain.equalsIgnoreCase((const char *)pgm_read_word(&(key_cmd.read_salinity))) || read_all_param)
        json_buffer_msg[F("salinity")] = Sensor::getSalinity();

    // specific gravity of sea water
    if (plain.equalsIgnoreCase((const char *)pgm_read_word(&(key_cmd.read_specific_of_gravity))) || read_all_param)
        json_buffer_msg[F("SoG")] = Sensor::getSpecifivGravity();

    // Total Dissolved Solid 
    if (plain.equalsIgnoreCase((const char *)pgm_read_word(&(key_cmd.read_tds))) || read_all_param)
        json_buffer_msg[F("TDS")] = Sensor::getTDS();

    // Dissolved oxygen in percent
    if (plain.equalsIgnoreCase((const char *)pgm_read_word(&(key_cmd.read_dissolved_oxygen_percent))) || read_all_param)
        json_buffer_msg[F("DO_%")] = Sensor::getDO_percent();

    // Dissolved oxygen in mg/l
    if (plain.equalsIgnoreCase((const char *)pgm_read_word(&(key_cmd.read_dissolved_oxygen_mgl))) || read_all_param){
        json_buffer_msg.createNestedObject(F("DO_mgl"));
        json_buffer_msg[F("DO_mgl")][F("value")] = Sensor::getDO_mgl();
        json_buffer_msg[F("DO_mgl")][F("stdev")] = Sensor::getDO_stdev();
        json_buffer_msg[F("DO_mgl")][F("stable")] = Sensor::isDOStable();
    }

    // water temperature
    if (plain.equalsIgnoreCase((const char *)pgm_read_word(&(key_cmd.read_water_temperature))) || read_all_param)
        json_buffer_msg[F("Water Temp")] = Sensor::getWaterTemperature();

    // elevation
    if ( plain.equalsIgnoreCase((const char *)pgm_read_word(&(key_cmd.read_elevation))) )
        json_buffer_msg[F("elevation")] = deviceParameter.elevation;

    // air pressure
    if (plain.equalsIgnoreCase((const char *)pgm_read_word(&(key_cmd.read_air_pressure))))
        json_buffer_msg[F("air_pressure")] = Sensor::getAirPressure();

    // take calibration file
    if (plain.equalsIgnoreCase((const char *)pgm_read_word(&(key_cmd.read_calibration_file))))
    {
        json_buffer_msg.createNestedObject(F("Cal"));
        json_buffer_msg[F("Cal")].createNestedObject(F("pH"));
        json_buffer_msg[F("Cal")][F("pH")][F("gain")] = deviceParameter.pH_calibration_parameter.slope;
        json_buffer_msg[F("Cal")][F("pH")][F("offset")] = deviceParameter.pH_calibration_parameter.offset;

        json_buffer_msg[F("Cal")].createNestedObject(F("DO"));
        json_buffer_msg[F("Cal")][F("DO")][F("gain")] = deviceParameter.DO_calibration_parameter.slope;
        json_buffer_msg[F("Cal")][F("DO")][F("offset")] = deviceParameter.DO_calibration_parameter.offset;

        json_buffer_msg[F("Cal")].createNestedObject(F("EC"));
        json_buffer_msg[F("Cal")][F("EC")][F("gain")] = deviceParameter.EC_calibration_parameter.slope;
        json_buffer_msg[F("Cal")][F("EC")][F("offset")] = deviceParameter.EC_calibration_parameter.offset;
    }

    

    // Sensor::sens.airPressure_in_kpa
        // serial number
    if (plain.equalsIgnoreCase((const char *)pgm_read_word(&(key_cmd.get_sn))))
    {
        String buf_sn="";
        for (uint8_t l = 0; l < UniqueIDsize; l++){
            buf_sn += String(serialDevice[l],HEX);
        }
        json_buffer_msg[F("SN")] = buf_sn;
    }
    else if (plain.equalsIgnoreCase((const char *)pgm_read_word(&(key_cmd.reset_cal_file))))
    {
        if ( resetMemory() )
            json_buffer_msg[(const char *)pgm_read_word(&(key_cmd.reset_cal_file))] = F("success");
        else
            json_buffer_msg[(const char *)pgm_read_word(&(key_cmd.reset_cal_file))] = F("failed");
        Sensor::initSensorPrecision();
    }
        
    // restart this module
    else if (plain.equalsIgnoreCase((const char *)pgm_read_word(&(key_cmd.restart))))
        reset_by_cmd = true;

    else if (plain.equalsIgnoreCase((const char *)pgm_read_word(&(key_cmd.ping))))
        json_buffer_msg[F("response")] = F("ok");
    else
    {
        if (json_buffer_msg.isNull())
            json_buffer_msg[F("response")] = plain + F(" unknown command");
    }
    json_str = "";
    serializeJson(json_buffer_msg, json_str);
    json_buffer_msg.clear();
}

void serial_com::parsingToSetParam(const String &plain, String &json_str)
{
    // creating json base
    StaticJsonDocument<buffer_serial_len> json_buffer_msg;
    json_buffer_msg.clear();
    if (json_str.length() > 3)
    {
        DeserializationError error = deserializeJson(json_buffer_msg, json_str);
        if (error)
        {
            json_buffer_msg.clear();
        }
    }
    // set elevation
    if (plain.equalsIgnoreCase((const char *)pgm_read_word(&(key_cmd.set_elevation))))
    {
        deviceParameter.elevation = json_buffer_msg[F("set_elevation")].as<float>();
    }
}


void serial_com::calibration_done(String &bufStr, const char* sens_type, const char* status)
{
    StaticJsonDocument<256> json_buffer_msg;
    json_buffer_msg.clear();
    DeserializationError error = deserializeJson(json_buffer_msg, bufStr);
    if (error)
        json_buffer_msg.clear();
    json_buffer_msg.createNestedObject(calibration_key);
    json_buffer_msg[calibration_key][sens_type] = status;
    serializeJson(json_buffer_msg, bufStr);
    json_buffer_msg.clear();
}