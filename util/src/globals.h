#pragma once

#include <stdint.h>

// attempt to get rid of #ifdef'ing
#ifdef VCTCXO
const bool VCTCXO_MODE = true;
#else
const bool VCTCXO_MODE = false;
#endif


#define ISUSED __attribute__((__unused__))

#define INLINE __attribute__((always_inline))

ISUSED static const char* g_multicastIp = "225.168.1.102";
const uint16_t g_multicastPort = 45654;

const int g_serverPingPeriod = 5000;
const int g_serverPingTimeout = 6000;

const int g_clientPingPeriod = 5000;
const int g_clientPingTimeout = 6000;

const int g_clientReconnectPeriod = 3000;

const int g_statusReport = 120000;

static const int TIMEROFF = -1;
const int TIMER_20MS = 20;
const int TIMER_1SEC = 1000;
const int TIMER_10SECS = 10 * 1000;
const int TIMER_5MIN = 5 * 60 * 1000;

static const int64_t NS_IN_SEC = 1000000000LL;
static const int64_t NS_IN_MSEC = 1000000LL;
static constexpr double NS_IN_SEC_F = 1000000000.0;
static constexpr double NS_IN_MSEC_F = 1000000.0;

enum DevelopmentMask
{
    None                        = 0x00,
    SameHost                    = 0x01,
    OnBailingOut                = 0x02,
    AnalysisAppendToSummary     = 0x04,
    SaveMeasurements            = 0x08,
    SaveClientSummaryLines      = 0x10,
    TurboMeasurements           = 0x20
};

// #define TRACE_TCP_COMMANDS
