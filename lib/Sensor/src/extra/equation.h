#ifndef _equation_h_
#define _equation_h_

#include "Arduino.h"

static inline float getPressureFrom(float temp, float elevation)
{
    temp += 273.15;
    const float Po = 101325;      // in Pa
    const float M = 0.02896968;   // kg/mol
    const float Ro = 8.314462618; // J/(mol.K)
    const float g = 9.80665;      // m/s^2

    float pressure = Po * exp(-(g * elevation * (float)M) / (temp * Ro));
    pressure /= 1000.f;
    return pressure;
}

static inline float conductivityTempCompensation(float C, float T)
{
    // C = conductivity, T = temperature deg C
    float ret;
    ret = C / (1 + (0.02 * (T - 25.00)));
    if (ret < 0.00)
        ret = 0;
    return (ret);
}

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

const float p0_a_[6] __attribute__((section(".rodata"))) = {999.842594, 0.06793952, -0.00909529, 1.001685E-4, -1.120083E-6, 6.536332E-9};
static inline float density_salt_water(float salinity, float temperature, float pressureInPa)
{
    pressureInPa *= 1E-5; // convert to bars
    float p0, ASP, BSP;
    p0 = p0_a_[0] + (p0_a_[1] * temperature) + (p0_a_[2] * pow(temperature, 2)) + (p0_a_[3] * pow(temperature, 3)) + (p0_a_[4] * pow(temperature, 4)) + (p0_a_[5] * pow(temperature, 5));
    ASP = 0.824493 + (-0.0040899 * temperature) + (7.6438E-5 * pow(temperature, 2)) + (-8.2467E-7 * pow(temperature, 3)) + (5.3875E-9 * pow(temperature, 4));
    BSP = -0.00572466 + (1.0227E-4 * temperature) + (-1.6546E-6 * pow(temperature, 2));
    p0 += (ASP * salinity) + (BSP * pow(salinity, 1.5)) + (4.8314E-4 * pow(salinity, 2));

    float kw, Aw, Bw;
    kw = 19652.21 + (148.4206 * temperature) + (-2.327105 * pow(temperature, 2)) + (0.01360477 * pow(temperature, 3)) + (-5.155288E-5 * pow(temperature, 4));
    Aw = 3.239908 + (0.00143713 * temperature) + (1.16092E-4 * pow(temperature, 2)) + (-5.77905E-7 * pow(temperature, 3));
    Bw = 8.50935E-5 + (-6.12293E-6 * temperature) + (5.2787E-8 * pow(temperature, 2));

    kw +=
        ((54.6746 + (-0.603459 * temperature) + (0.0109987 * pow(temperature, 2)) + (-6.167E-5 * pow(temperature, 3))) * salinity) +
        ((0.07944 + (0.016483 * temperature) + (-5.30094E-4 * pow(temperature, 2))) * pow(salinity, 1.5));

    Aw +=
        ((0.0022838 + (-1.0981E-5 * temperature) + (-1.6078E-6 * pow(temperature, 2))) * salinity) +
        (1.91075E-4 * pow(salinity, 1.5));

    Bw += (-9.9348E-7 + (2.0816E-8 * temperature) + (9.1697E-10 * pow(temperature, 2)) * salinity);

    kw += (Aw * pressureInPa) + (Bw * pow(pressureInPa, 2));
    p0 /= (1 - (pressureInPa / kw));
    return p0;
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