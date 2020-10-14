#include "board_io.h"
#include <avr/boot.h>


uint8_t tentacles_pin[4] = {S0_tentacles_pin,
                            S1_tentacles_pin,
                            E1_tentacles_pin,
                            E2_tentacles_pin, };
char serialDevice[UniqueIDsize+1] = "";

deviceParam deviceParameter = {0};

void reset_device_parameter(void)
{
    deviceParameter.elevation  = 5.0f;
    deviceParameter.pH_calibration_parameter = (calibrationParam){.slope = 1, .offset = 0};
    deviceParameter.DO_calibration_parameter = (calibrationParam){.slope = 1, .offset = 0};
    deviceParameter.EC_calibration_parameter = (calibrationParam){.slope = 1, .offset = 0};

    deviceParameter.pH_precision = 0.05f;   // pH value, stdev
    deviceParameter.DO_precision = 0.05f;   // mgl, stdev
    deviceParameter.EC_precision = 100.f;   // uS/cm, stdev
    deviceParameter.water_temperature_precision = 0.025f;   // degree celcius, stdev
}

void setup_bsp(void)
{
    for( uint8_t pins = 0; pins < 4; pins++) {
        pinMode(tentacles_pin[pins], OUTPUT);
    }
    pinMode(EZO_RX_PIN, INPUT_PULLUP);
    pinMode(EZO_TX_PIN, OUTPUT);

    for (uint8_t i = 0; i < UniqueIDsize; i++)
    {
        serialDevice[i] = boot_signature_byte_get(0x0E + i + (UniqueIDsize == 9 && i > 5 ? 1 : 0));
        serialDevice[i] += UniqueIDsize - 8;
    }
    serialDevice[UniqueIDsize]='\0';

}

void tentacles_open_channel(uint8_t ch)
{
    /**
     *  convert ch value as each bit 
     *  control bits representate by bits-position in <ch>
     *  bit 0 : S0_tentacles_pin
     *      1 : S1_tentacles_pin
     *      2 : E1_tentacles_pin
     * with the E2_tentacles_pin is the negation of E1_tentacles_pin
    */
    if ( ch > 7 )
        return;
    for( uint8_t bits = 0; bits < 3; bits++)
    {
        digitalWrite(tentacles_pin[bits],bitRead(ch,bits));
        if( bits == 2)
            digitalWrite(tentacles_pin[bits+1], !bitRead(ch,bits));

    };
}
