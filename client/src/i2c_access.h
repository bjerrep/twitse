#pragma once

#include <QByteArray>


class I2C_Access
{
public:
    I2C_Access(int bus);
    ~I2C_Access();

    double readTemperature();
    void writeLTC2606(uint16_t value);

private:
    bool m_valid = false;
    int m_descriptor;
};

