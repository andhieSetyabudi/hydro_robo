# hydro_robo
water quality measurement for hydroponic
Reading WQM - Sensor
1. pH
2. Dissolved oxygen
3. Conductivity
4. Temperature


Command format : JSON

parse the command by categories : 
1. "cmd" for command and data request
  with keyword :
    "ping"    : to check response
    "get_sn"  : to get Serial Number of Board
    "restart" : to restarting modules
 for uncalibrated value
    "get_pH_uncal"    : to get uncalibrated of pH value.
    "get_ec_uncal"    : to get uncalibrated of conductivity value
    "get_do_uncal"    : to get uncalibrated of Dissolved_oxygen ( in percent ) value
for calibrated value
    "get_all"         : to get reading value of pH, salinity, dissolved-oxygen and temperature. all value has been calibrated
    "get_ph"          : to get pH value
    "get_ec"          : to get conductivity value in us/cm
    "get_salinity"    : to get salinity value in ppt
    "get_tds"         : to get tds value
    "get_sog"         : to get specific gravity of sea water
    "get_do_mgl"      : to get DO value in mg/l
    "get_do_%"        : to get DO value in %
    "get_water_temp"  : to get temperature of the water sample
    "get_elevation"   : to get elevation of the modules, the value has been stored in internal memory
    "get_air_pressure" : to get air pressure around the modules / sensor-probes
    "get_cal_file"    : to get the calibration' file
   
   json format for cmd :
   {"cmd":<keyword>} or {"cmd":[keyword1, keyword2, ..]}
   e.g:
   {"cmd":"ping"} => response is : {"response":"ok"}
  
   {"cmd":["get_sn", "get_ph"]}=> response is :   {"SN":"56353234313317a12","pH":{"value":7.00299,"stdev":5.026305e-7,"stable":true}}

    
2. "cal" for calibration-function, 
   json format for the calibration is :
   for single point of calibration
    {"cal":{"type":<sensor,"cp":<current value of uncalibrated>,"sp":<reference value>}}
    for multi-point of calibration
    {"cal":{"type":<sensor,"cp":[cp_val1, cp_val2, cp_val3, ...],"sp":[sp_val1, sp_val2, sp_val3, ...]}}
    
    for example :
    {"cal":{"type":"ec","sp":[0, 1413, 12880],"cp":[36.38, 2276, 16400]}}
    -> calibrating EC sensor with reference 0us/cm, 1413us/cm and 12880us/cm
