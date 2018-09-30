#ifdef VCTCXO

#include "globals.h"
#include "systemtime_vctcxo.h"
#include "log.h"

#include <time.h>
#include <sys/time.h>


void SystemTime::adjustSystemTime_ns(int64_t adjustment_ns)
{
    m_monotonicClockOffset += adjustment_ns;
    m_resetTime += adjustment_ns;
}


void SystemTime::reset()
{
    m_resetTime = getRawSystemTime();
}


double SystemTime::getRunningTime_secs()
{
    return (getRawSystemTime() - m_resetTime) / NS_IN_SEC_F;
}


int64_t SystemTime::getWallClock()
{
    struct timespec ts = {0};
    clock_gettime(CLOCK_REALTIME, &ts);
    uint64_t sec = ts.tv_sec;
    uint64_t nsec = ts.tv_nsec;
    return sec * NS_IN_SEC + nsec;
}


void SystemTime::setWallclock(int64_t epoch)
{
    struct timespec ts = {(__time_t) (epoch / NS_IN_SEC), (__syscall_slong_t) (epoch % NS_IN_SEC)};
    int err = clock_settime(CLOCK_REALTIME, &ts);
    if (err < 0)
    {
        trace->critical("unable to set clock (insufficient priviledges?)");
    }
}


// just abusing the convience that this is global getter/setters in VCTCXO mode.
// SystemTime in VCTCXO mode has no concept of a 'ppm' offset.

void SystemTime::setPPM(double lsb)
{
    m_dac = lsb;
}


double SystemTime::getPPM()
{
    return m_dac;
}

#endif
