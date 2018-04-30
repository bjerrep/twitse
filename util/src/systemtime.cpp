#include "systemtime.h"
#include <time.h>
#include <sys/time.h>
#include "log.h"

double s_ppm = 0.0;
int64_t s_ppmTime = 0;
int presc = 0;
int64_t s_resetTime = 0;
bool s_ppmInitialized = false;


/// epoch with ns resolution in bash: date +%s%N
///
int64_t SystemTime::getKernelSystemTime()
{
    struct timespec ts = {0};
    clock_gettime(CLOCK_REALTIME, &ts);
    uint64_t sec = ts.tv_sec;
    uint64_t nsec = ts.tv_nsec;

    return sec * NS_IN_SEC + nsec;
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


int64_t SystemTime::getSystemTime(int64_t adjustment_ns)
{
    int64_t systime = getKernelSystemTime();

    if (!s_ppmInitialized)
        return systime;

    int64_t offset = - (systime - s_ppmTime) * s_ppm  / 1000000.0;

    if ((abs(offset) > 500000) || adjustment_ns)
    {
        const int empirical_rpi3_correction = 10000;
        int64_t new_kernel_time = systime + offset + adjustment_ns + empirical_rpi3_correction;
        setKernelSystemTime(new_kernel_time);
        s_ppmTime = new_kernel_time;
        return new_kernel_time;
    }
    return systime + offset;
}


void SystemTime::adjustSystemTime(int64_t adjustment_ns)
{
    if (!s_ppmInitialized)
    {
        SystemTime::setKernelSystemTime(SystemTime::getKernelSystemTime() + adjustment_ns);
    }
    else
    {
        int64_t systime = SystemTime::getSystemTime(adjustment_ns);
        s_ppmTime = systime;
    }
}


double SystemTime::getSystemTime_sec()
{
    return SystemTime::getSystemTime() / SystemTime::NS_IN_SEC_F;
}


void SystemTime::reset()
{
    s_ppm = 0.0;
    s_ppmTime = 0;
    s_resetTime = getKernelSystemTime();
    s_ppmInitialized = false;
}


double SystemTime::getRunningTime_secs()
{
    return (getKernelSystemTime() - s_resetTime) / SystemTime::NS_IN_SEC_F;
}


void SystemTime::setPPM(double ppm)
{
    s_ppm += ppm;

    if (!s_ppmInitialized)
    {
        s_ppmTime = getSystemTime();
        s_ppmInitialized = true;
    }
}


double SystemTime::getPPM()
{
    return s_ppm;
}
