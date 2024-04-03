#pragma once

#include <stdint.h>

class I2C_Access
{
public:

    static I2C_Access* I2C();

    void exit();

    bool readTemperature(double& temperature);

    void setFixedVCTCXO_DAC(bool fixed);

    bool getFixedVCTCXO_DAC() const;

    void writeVCTCXO_DAC(uint16_t dac);

    bool adjustVCTCXO_DAC(int32_t relative_dac);

    uint16_t getVCTCXO_DAC();

private:
    I2C_Access(int bus = 1);
    void writeMAX5217BGUA(uint16_t value);
    void writeLTC2606IDD1(uint16_t value);

private:
    static I2C_Access* m_self;
    bool m_valid = false;
    int m_descriptor;
    uint16_t m_dac = 0x8000;
    bool m_fixed_dac = false;
};
