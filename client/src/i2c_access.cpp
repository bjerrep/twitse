#include "i2c_access.h"
#include "log.h"
#include "systemtime.h"

#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#define I2CBUFPTR char
#ifndef I2C_FUNC_I2C
#include <linux/i2c.h>
#undef I2CBUFPTR
#define I2CBUFPTR uint8_t
#endif


I2C_Access::I2C_Access(int bus)
{
    const std::string device = fmt::format("/dev/i2c-{}", bus);
    m_descriptor = open(device.c_str(), O_RDWR);

    if (m_descriptor >= 0)
    {
        m_valid = true;
        trace->info("open i2c bus {} successfull", device);
    }
    else
    {
        trace->info("unable to open {}, disabling i2c", device);
    }
}


I2C_Access::~I2C_Access()
{
    close(m_descriptor);
}


void I2C_Access::writeMAX5217BGUA(uint16_t value)
{
    const uint8_t MAX5217 = 0x1D;

    uint8_t tx[] = {0x01, (uint8_t) (value >> 8), (uint8_t) (value & 0x00FF)};

    if (ioctl(m_descriptor, I2C_SLAVE, MAX5217) < 0)
    {
        trace->warn("unable to setup i2c:0x{:02x}", MAX5217);
        return;
    }

    write(m_descriptor, tx, 3);
}


void I2C_Access::writeLTC2606IDD1(uint16_t value)
{
    const uint8_t LTC2606 = 0x10;

    uint8_t tx[] = {0x30, (uint8_t) (value >> 8), (uint8_t) (value & 0x00FF)};

    if (ioctl(m_descriptor, I2C_SLAVE, LTC2606) < 0)
    {
        trace->warn("unable to setup i2c:0x{:02x}", LTC2606);
        return;
    }

    write(m_descriptor, tx, 3);
}


double I2C_Access::readTemperature()
{
    uint8_t temperaturePointer = 0;
    int16_t reading;

    const uint8_t DS1775R1_TR = 0x49;

    struct i2c_msg msgs[2] = // uses repeated start
    {
    {
        .addr = DS1775R1_TR,
        .flags = 0,
        .len = 1,
        .buf = (I2CBUFPTR*) &temperaturePointer
    },
    {
        .addr = DS1775R1_TR,
        .flags = I2C_M_RD,
        .len = 2,
        .buf = (I2CBUFPTR*) &reading
    }
};

    struct i2c_rdwr_ioctl_data data = {
        .msgs = msgs,
        .nmsgs = 2
    };

    if (ioctl(m_descriptor, I2C_RDWR, &data) < 0)
    {
        trace->warn("luhab board temperature reading failed");
        return 0.0;
    }

    double temperature = __bswap_16(reading) / 256.0;
    return temperature;
}


void I2C_Access::writeVCTCXO_DAC(uint16_t value)
{
    s_systemTime->setPPM(value);
    writeMAX5217BGUA(value);
    //writeLTC2606IDD1(value);
}

