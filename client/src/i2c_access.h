#pragma once

#include <stdint.h>

class I2C_Access
{
public:
    I2C_Access(int bus);
    ~I2C_Access();

    double readTemperature();

    void setFixedVCTCXO_DAC(bool fixed);

    void writeVCTCXO_DAC(uint16_t value);

    static uint16_t getVCTCXO_DAC();

private:
    void writeMAX5217BGUA(uint16_t value);
    void writeLTC2606IDD1(uint16_t value);

private:
    bool m_valid = false;
    int m_descriptor;
    static uint16_t m_dac;
    static bool m_fixed_dac;
};
