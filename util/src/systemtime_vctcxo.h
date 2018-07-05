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

        return sec * NS_IN_SEC + nsec + m_monotonicClockOffset;
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
    // Added at startup as a constant difference between wall clock and the unpredictable CLOCK_MONOTONIC_RAW.
    // In case the wall clock is sane this will yield sane epoch measurements to look at when developing.
    int64_t m_monotonicClockOffset = 0;
};
