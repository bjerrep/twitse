
#ifndef VCTCXO

#include "systemtime_std.h"
#include "log.h"


void SystemTime::setSystemTime(int64_t epoch)
{
    struct timespec ts = {(__time_t) (epoch / NS_IN_SEC), (__syscall_slong_t) (epoch % NS_IN_SEC)};
    int err = clock_settime(CLOCK_REALTIME, &ts);
    if (err < 0)
    {
        trace->critical("unable to set clock (insufficient priviledges?)");
    }
}


void SystemTime::adjustSystemTime_ns(int64_t adjustment_ns)
{
    if (!s_ppmInitialized)
    {
        setSystemTime(getRawSystemTime() + adjustment_ns);
        s_resetTime += adjustment_ns;
    }
    else
    {
        int64_t systime = getUpdatedSystemTime(adjustment_ns);
        s_ppmTime = systime;
    }
}


void SystemTime::reset()
{
    s_ppm = 0.0;
    s_ppmTime = 0;
    s_resetTime = getRawSystemTime();
    s_ppmInitialized = false;
}


double SystemTime::getRunningTime_secs()
{
    return (getRawSystemTime() - s_resetTime) / NS_IN_SEC_F;
}


void SystemTime::setWallclock(int64_t epoch)
{
    trace->critical("This is wrong. setWallclock is only implemented for VCTCXO mode");
}


void SystemTime::setPPM(double ppm)
{
    s_ppm += ppm;

    if (!s_ppmInitialized)
    {
        s_ppmTime = getUpdatedSystemTime();
        s_ppmInitialized = true;
    }
}


double SystemTime::getPPM()
{
    return s_ppm;
}

#endif
