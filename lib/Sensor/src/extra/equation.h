#ifndef _equation_h_
#define _equation_h_

#include "Arduino.h"

const float _a_[6] = {0.0080, -0.1692, 25.3851, 14.0941, -7.0261, 2.7081};
const float _b_[6] = {0.0005, -0.0056, -0.0066, -0.0375, 0.0636, -0.0144};
const float _c_[5] = {0.6766097, 0.0200564, 0.0001104259, -0.00000069698, 0.0000000010031};


static inline float condToSal(float C, float T)
{
    double salinity, ds, rt, Rt, ratio;
    if (C <= 0)
        salinity = 0;
    else
    {
        ratio = C / 42914.0f;
        double dS1 = (T - 15.0) / (1.0f + 0.0162 * (T - 15.0f));
        rt = (_c_[0] + (_c_[1] * T) + (_c_[2] * pow(T, 2)) + (_c_[3] * pow(T, 3)) + (_c_[4] * pow(T, 4)));
        Rt = ratio / rt;
        double dS2 = _b_[0] + (_b_[1] * pow(Rt, 0.5)) + (_b_[2] * Rt) + (_b_[3] * pow(Rt, 1.5)) + (_b_[4] * pow(Rt, 2)) + (_b_[5] * pow(Rt, 2.5));
        ds = dS1 * dS2;
        salinity = _a_[0] + (_a_[1] * pow(Rt, 0.5)) + (_a_[2] * Rt) + (_a_[3] * pow(Rt, 1.5)) + (_a_[4] * pow(Rt, 2)) + (_a_[5] * pow(Rt, 2.5)) + ds;
    }
    return ((float)salinity); // in ppt
}

static inline void saturationDO(float *result, float tempInC, float pressureInKpa)
{
    float K, atm, theta, c_note, Pwv;
    K = tempInC + 273.15;           // convert Celcius to Kelvin
    atm = pressureInKpa / 101.325f; // convert kPa to atmosphere

    c_note = exp(7.7117 - 1.31403 * log(tempInC + 45.93)); // equilibrium oxygen concentration at standard pressure of 1atm, mg/L
    Pwv = exp(11.8571 - (3840.70 / K) - (216961 / powf(K, 2.0f)));
    theta = 0.000975 - (1.426e-5 * tempInC) + (6.436e-8 * tempInC * tempInC);
    *result = c_note * atm;
    *result *= (((1 - Pwv / atm) * (1 - theta * atm)) / ((1 - Pwv) * (1 - theta)));
}

static inline float saturationDOvalue( float tempInC, float pressureInkPa, float ECInus )
{
    float DO = 0;
    saturationDO(&DO, tempInC, pressureInkPa);
    if (ECInus > 0) // calculate correction factor of salinity
    {
        float sc, K;
        K = tempInC + 273.15; // to kelvin
        sc = 5.572E-4 * ECInus + 2.02E-9 * ECInus * ECInus;
        DO *= exp(-1 * sc * (0.017674f + (-10.754f + 2140.7f / K) / K));
    }
    return DO;
}


#endif