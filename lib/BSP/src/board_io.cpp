#include "board_io.h"


uint8_t tentacles_pin[4] = {S0_tentacles_pin,
                            S1_tentacles_pin,
                            E1_tentacles_pin,
                            E2_tentacles_pin, };
void setup_bsp(void)
{
    for( uint8_t pins = 0; pins < 4; pins++) {
        pinMode(tentacles_pin[pins], OUTPUT);
    }
    pinMode(EZO_RX_PIN, INPUT_PULLUP);
    pinMode(EZO_TX_PIN, OUTPUT);
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
