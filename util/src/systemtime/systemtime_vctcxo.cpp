#include "../systemtime.h"
#include "../log.h"

#include <time.h>
#include <sys/time.h>


uint16_t s_dac = 0;
int64_t s_resetTime = 0;
int64_t s_rawOffset = 0;


int64_t SystemTime::getKernelSystemTime()
{
    struct timespec ts = {0};

    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);

    uint64_t sec = ts.tv_sec;
    uint64_t nsec = ts.tv_nsec;

    return sec * NS_IN_SEC + nsec + s_rawOffset;
}


void SystemTime::setKernelSystemTime(int64_t epoch)
{
    struct timespec ts = {(__time_t) (epoch / NS_IN_SEC), (__syscall_slong_t) (epoch % NS_IN_SEC)};
    int err = clock_settime(CLOCK_REALTIME, &ts);
    if (err < 0)
    {
        trace->critical("unable to set clock (insufficient priviledges?)");
    }
}


int64_t SystemTime::getSystemTime(int64_t /*adjustment_ns*/)
{
    return getKernelSystemTime();
}


void SystemTime::adjustSystemTime(int64_t adjustment_ns)
{
    s_rawOffset += adjustment_ns;
    s_resetTime += adjustment_ns;
}


double SystemTime::getSystemTime_sec()
{
    return SystemTime::getSystemTime() / SystemTime::NS_IN_SEC_F;
}


void SystemTime::reset()
{
    s_dac = 0;
    s_resetTime = getKernelSystemTime();
}


double SystemTime::getRunningTime_secs()
{
    return (getKernelSystemTime() - s_resetTime) / SystemTime::NS_IN_SEC_F;
}

// just abusing the convience that this is static getter/setters in VCTCXO mode.
// SystemTime in VCTCXO mode has no concept of a 'ppm' offset.

void SystemTime::setPPM(double lsb)
{
    s_dac = lsb;
}


double SystemTime::getPPM()
{
    return s_dac;
}
