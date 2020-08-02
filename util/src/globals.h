#pragma once

#include <stdint.h>

// attempt to get rid of #ifdef'ing
#ifdef VCTCXO
/// For the server this means that it should track the (ntp) wall clock and that it additionally can be
/// started with a '--vctcxo' in case the server have a vctcxo as the clients do that can be used for tracking.
const bool VCTCXO_MODE = true;
#else
/// The server can't use ntp and as consequence the wall clock error will drift toward infinity.
const bool VCTCXO_MODE = false;
#endif

#define ISUSED __attribute__((__unused__))

#define INLINE __attribute__((always_inline))

ISUSED static const char* g_multicastIp = "225.168.1.102";
const uint16_t g_multicastPort = 45654;

const uint16_t g_websocketPort = 12343;

const int g_serverPingPeriod = 5000;
const int g_serverPingTimeout = 6000;

const int g_clientPingPeriod = 5000;
const int g_clientPingTimeout = 6000;

const int g_clientReconnectPeriod = 3000;

const int g_statusReport = 120000;

const int TIMEROFF = -1;
const int TIMER_20MS = 20;
const int TIMER_1SEC = 1000;
const int TIMER_10SECS = 10 * 1000;
const int TIMER_5MIN = 5 * 60 * 1000;
const int TIMER_ONE_DAY = 24 * 60 * 60 * 1000;

const int64_t NS_IN_SEC = 1000000000LL;
const int64_t NS_IN_MSEC = 1000000LL;
static constexpr double NS_IN_SEC_F = 1000000000.0;
static constexpr double NS_IN_MSEC_F = 1000000.0;
const int64_t US_IN_SEC_F = 1000000.0;
const int64_t MSEC_IN_HOUR = 1000 * 60 * 60;
const int64_t HOURS_IN_WEEK = 24 * 7;

const int NOF_INITIAL_PPM_MEASUREMENTS = 3;

enum DevelopmentMask
{
    None                        = 0x00,
    SameHost                    = 0x01,
    SaveOnBailingOut                = 0x02,
    AnalysisAppendToSummary     = 0x04,
    SaveMeasurements            = 0x08,
    SaveClientSummaryLines      = 0x10,
    TurboMeasurements           = 0x20,
    SamplePeriodSweep           = 0x40,
    SaveMeasurementsSingle      = 0x80
};

#define develTurbo (g_developmentMask & DevelopmentMask::TurboMeasurements)
#define develPeriodSweep (g_developmentMask & DevelopmentMask::SamplePeriodSweep)
