#pragma once

#include "globals.h"
#include <cstdint>

#include <time.h>
//#include <sys/time.h>


class SystemTime
{
public:
    SystemTime(bool isServer) : m_server(isServer) {}

    void reset();

    int64_t INLINE getRawSystemTime()
    {
        struct timespec ts;

        clock_gettime(CLOCK_MONOTONIC_RAW, &ts);

        uint64_t sec = ts.tv_sec;
        uint64_t nsec = ts.tv_nsec;

        return sec * NS_IN_SEC + nsec + m_rawOffset;
    }

    int64_t INLINE getUpdatedSystemTime(int64_t adjustment_ns = 0)
    {
#ifdef CLIENT_USING_REALTIME
        if (m_server)
            return getRawSystemTime();
        else
            return getWallClock();
#else
        return getRawSystemTime();
#endif
    }
    
    int64_t getWallClock()
    {
        struct timespec ts = {0};
        clock_gettime(CLOCK_REALTIME, &ts);
        uint64_t sec = ts.tv_sec;
        uint64_t nsec = ts.tv_nsec;
        return sec * NS_IN_SEC + nsec;
    }

    void adjustSystemTime(int64_t adjustment_ns);

    void setPPM(double ppm);
    double getPPM();

    double getRunningTime_secs();

private:    
    int64_t getKernelSystemTime();

private:
    bool m_server;
    uint16_t m_dac = 0;
    int64_t m_resetTime = 0;
    // set on the client by the server to get rid of any large and static offset that
    // makes it awfull to look at the timing values.
    int64_t m_rawOffset = 0;
};