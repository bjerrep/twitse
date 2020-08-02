#pragma once

#include "log.h"
#include "globals.h"
#include <cstdint>

#include <time.h>

/*
vctcxo

The system base clock is the server CLOCK_MONOTONIC_RAW clock, a clock that
runs directly of the processor xtal. If the server is running NTP then its
wall clock will drift with regard to the CLOCK_MONOTONIC_RAW clock. The obvious
solution to this would be to run the server of a vctcxo tracking the NTP wall
clock like its done on the clients, but thats not made yet.

On the clients the reference time will their CLOCK_MONOTONIC_RAW as well. Since
the server and individual client CLOCK_MONOTONIC_RAW values most likely originate
from different planets a individuel static offset is sent to the clients during
startup so that

   server.CLOCK_MONOTONIC_RAW = client[n].CLOCK_MONOTONIC_RAW + client[n].offset   (1)

This expression is what the server continuesly attempts to ensure is true.

The client wallclock is running alongside its CLOCK_MONOTONIC_RAW since it doesn't
have NTP or the like running to spoil things. During the start phase the server
establishes a single fixed value for the wall clock offset as

   wall_offset = server.CLOCK_REALTIME - server.CLOCK_MONOTONIC_RAW                (2)

This will be sent to clients and they will be able to adjust their CLOCK_REALTIME
clock accordingly. So user space applications running on the clients should run
in time sync also in the case they are based on the wall clocks.

What remains is that the client wallclock will slowly drift away from the client
wallclocks. The future might bring a vctcxo mode on the server as well so the server
will be able to adjusts it vctcxo to track NTP.

*/


class SystemTime
{
public:
    SystemTime(bool isServer) : m_server(isServer) {}

    void reset();

    /// not sure if the inline has any effect but it mandates that the implementation is
    /// here in the declaration.
    ///
    int64_t INLINE getRawSystemTime_ns()
    {
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
        int64_t time = ts.tv_sec * NS_IN_SEC + ts.tv_nsec + m_rawClockOffset;
        return time;
    }


    int64_t INLINE getUpdatedSystemTime()
    {
#ifdef CLIENT_USING_REALTIME
        if (m_server)
            return getRawSystemTime();
        else
            return getWallClock();
#else
        return getRawSystemTime_ns();
#endif
    }

    static int64_t getWallClock_ns();
    void setWallclock_ns(int64_t epoch);

    void adjustSystemTime_ns(int64_t adjustment_ns);

    void setPPM(double ppm);
    double getPPM();

    double getRunningTime_secs();

private:
    int64_t getKernelSystemTime();

private:
    bool m_server;
    int64_t m_resetTime = 0;
    /// Added at startup as a constant difference between wall clock and the unpredictable CLOCK_MONOTONIC_RAW.
    /// In case the wall clock is sane this will yield sane epoch measurements to look at when developing.
    int64_t m_rawClockOffset = 0;
    int64_t m_serverLastPPMSetTime = 0;
};
