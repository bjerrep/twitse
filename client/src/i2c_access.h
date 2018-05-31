#pragma once

#include <QByteArray>


class I2C_Access
{
public:
    I2C_Access(int bus);
    ~I2C_Access();

    double readTemperature();

    void writeVCTCXO_DAC(uint16_t value);

private:
    void writeMAX5217BGUA(uint16_t value);
    void writeLTC2606IDD1(uint16_t value);

private:
    bool m_valid = false;
    int m_descriptor;
};

