#ifdef VCTCXO

#include "globals.h"
#include "systemtime_vctcxo.h"
#include "log.h"

#include <time.h>
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


int64_t SystemTime::getWallClock()
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    int64_t sec = ts.tv_sec;
    int64_t nsec = ts.tv_nsec;
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


void SystemTime::setPPM(double server_ppm)
{
    if (!m_serverPPMRawInitialized)
    {
        m_serverLastPPMSetTime = getRawSystemTime();
        m_serverPPMRawInitialized = true;
    }
    else
    {
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
        int64_t time = ts.tv_sec * NS_IN_SEC + ts.tv_nsec + m_rawClockOffset;

        int64_t server_correction = (((double) time) - m_serverLastPPMSetTime) * (m_server_ppm/1000000.0);

        m_rawClockOffset += server_correction;
        m_serverLastPPMSetTime = time + server_correction;
    }
    m_server_ppm += server_ppm;
}


double SystemTime::getPPM()
{
   return m_server_ppm;
}

#endif
