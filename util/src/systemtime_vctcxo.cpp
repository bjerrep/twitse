#ifdef VCTCXO

#include "globals.h"
#include "systemtime_vctcxo.h"
#include "log.h"

#include <ctime>
#include <sys/time.h>

#include <QtGlobal>


void SystemTime::adjustSystemTime_ns(int64_t adjustment_ns)
{
    m_rawClockOffset += adjustment_ns;
}


void SystemTime::reset()
{
    m_resetTime = getRawSystemTime_ns();
}

/// For informational use on the server, the value is not corrected for xtal drift.
/// Once there is something in place that guarantees that the wall clock is valid
/// when starting the wall clock should be used instead.
///
double SystemTime::getRunningTime_secs()
{
    return (getRawSystemTime_ns() - m_resetTime) / NS_IN_SEC_F;
}


int64_t SystemTime::getWallClock_ns()
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return ts.tv_sec * NS_IN_SEC + ts.tv_nsec;
}


void SystemTime::setWallclock_ns(int64_t epoch)
{
    struct timespec ts = {(__time_t) (epoch / NS_IN_SEC), (__syscall_slong_t) (epoch % NS_IN_SEC)};
    int err = clock_settime(CLOCK_REALTIME, &ts);
    if (err < 0)
    {
        trace->critical("unable to set clock (insufficient priviledges?)");
    }
}


void SystemTime::setPPM(double server_ppm)
{
}


double SystemTime::getPPM()
{
    trace->error("getPPM invalid running with VCTCXO");
    return 0.0;
}

#endif
