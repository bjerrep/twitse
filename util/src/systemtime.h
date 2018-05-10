#pragma once

#include <cstdint>
#include <QString>

/* NOTE: this class has two different implementation files:
   systemtime_vctcxo.cpp  for LUHAB with dac controlled vctcxo
   systemtime_std.cpp     default standalone
*/

class SystemTime
{
public:
    static const int64_t NS_IN_SEC = 1000000000LL;
    static constexpr double NS_IN_SEC_F = 1000000000.0;

    static void reset();

    static int64_t getSystemTime(int64_t adjustment_ns = 0);

    static void adjustSystemTime(int64_t adjustment_ns);

    static void setPPM(double ppm);
    static double getPPM();

    static double getSystemTime_sec();
    static double getRunningTime_secs();

private:
    static int64_t getKernelSystemTime();
    static void setKernelSystemTime(int64_t epoch);
};


