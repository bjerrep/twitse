#pragma once

#include "globals.h"

#include <cstdint>
#include <cmath>
#include <time.h>
#include <sys/time.h>

class SystemTime
{
public:
    SystemTime(bool isServer) : m_server(isServer) {}

    void reset();

    inline int64_t INLINE getRawSystemTime()
    {
        struct timespec ts = {0};
        clock_gettime(CLOCK_REALTIME, &ts);
        uint64_t sec = ts.tv_sec;
        uint64_t nsec = ts.tv_nsec;

        return sec * NS_IN_SEC + nsec;
    }

    inline int64_t INLINE getUpdatedSystemTime(int64_t adjustment_ns = 0)
    {
        int64_t systime = getRawSystemTime();

        if (!s_ppmInitialized)
            return systime;

        int64_t offset = - (systime - s_ppmTime) * s_ppm  / 1000000.0;

        if ((std::abs(offset) > 500000) || adjustment_ns)
        {
            const int empirical_rpi3_correction = 10000;
            int64_t new_kernel_time = systime + offset + adjustment_ns + empirical_rpi3_correction;
            setSystemTime(new_kernel_time);
            s_ppmTime = new_kernel_time;
            return new_kernel_time;
        }
        return systime + offset;
    }

    int64_t getWallClock()
    {
        struct timespec ts = {0};
        clock_gettime(CLOCK_REALTIME, &ts);
        uint64_t sec = ts.tv_sec;
        uint64_t nsec = ts.tv_nsec;
        return sec * NS_IN_SEC + nsec;
    }

    // not implemented yet for non-VCTCXO builds.
    // Toying with the idea to replace the ifdef rubbish with runtime conditionals
    // leads to stuff like this which is unfortunately rubbish as well.
    void setWallclock(int64_t epoch);

    void adjustSystemTime_ns(int64_t adjustment_ns);

    void setPPM(double ppm);
    double getPPM();

    double getRunningTime_secs();

private:
    int64_t getKernelSystemTime();

    void setSystemTime(int64_t epoch); // fixit wallclock or systemtime ?

private:
    bool m_server;
    double s_ppm = 0.0;
    int64_t s_ppmTime = 0;
    int64_t s_resetTime = 0;
    bool s_ppmInitialized = false;
};
