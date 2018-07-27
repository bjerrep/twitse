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
