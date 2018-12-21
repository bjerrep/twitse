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
    m_resetTime = getRawSystemTime();
}

/// For informational use on the server, the value is not corrected for xtal drift.
/// Once there is something in place that guarantees that the wall clock is valid
/// when starting the wall clock should be used instead.
///
double SystemTime::getRunningTime_secs()
{
    return (getRawSystemTime() - m_resetTime) / NS_IN_SEC_F;
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
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    int64_t time = ts.tv_sec * NS_IN_SEC + ts.tv_nsec + m_rawClockOffset;

    if (!m_serverPPMRawInitialized)
    {
        m_serverLastPPMSetTime = time;
        m_serverPPMRawInitialized = true;
    }
    else
    {
        int64_t server_correction = (time - m_serverLastPPMSetTime) * m_server_slope;

        m_rawClockOffset += server_correction;
        m_serverLastPPMSetTime = time;
    }
    m_server_slope += server_ppm / 1000000.0;
}


double SystemTime::getPPM()
{
   return m_server_slope * 1000000.0;
}

#endif
