#include "lock.h"
#include "log.h"

extern DevelopmentMask g_developmentMask;
extern int g_clientPeriod;
extern int g_clientSamples;


Lock::Lock(const std::string &clientname)
    : m_clientName(clientname)
{
    trace->info("sample distribution is '{}'", m_distribution == BURST_SILENCE ? "burst/silence": "evenly distribution");
    if (g_developmentMask & DevelopmentMask::TurboMeasurements)
    {
        m_maxSamples /= 5;
    }
}


bool Lock::isHiLock() const
{
    return m_lockState == HILOCK;
}


bool Lock::isLock() const
{
    return m_lockState != UNLOCKED;
}


Lock::LockState Lock::getLockState() const
{
    return m_lockState;
}


int Lock::getInterMeasurementDelaySecs() const
{
    switch (m_distribution)
    {
    case EVENLY_DISTRIBUTED:
        return 0;
    case BURST_SILENCE:
        if (g_clientPeriod >= 0)
            return g_clientPeriod;
        return getMeasurementPeriodsecs() - getNofSamples() * getPeriodMSecs() / 1000.0;
    }
    return 0;
}


int Lock::getPeriodMSecs() const
{
    switch (m_distribution)
    {
    case EVENLY_DISTRIBUTED:
        return 1000 * getMeasurementPeriodsecs() / getNofSamples();
    case BURST_SILENCE:
        return g_minSampleInterval_ms;
    }
    return 0;
}


int Lock::getMeasurementPeriodsecs() const
{
    const int min_period = (m_maxSamples * g_minSampleInterval_ms) / 1000;

    if (g_clientPeriod >= 0)
        return g_clientPeriod + min_period;

    if (m_quality < 8)
    {
        return min_period;
    }

    int period = min_period + (20.0/12.0) * (m_quality - 8);
    if (period > 30)
    {
        return g_maxMeasurementPeriod_sec;
    }
    return period;
}


int Lock::getNofSamples() const
{
    if (g_clientSamples >= 0)
        return g_clientSamples;

    const int factor = (m_maxSamples - g_minNofSamples) / MAX_QUALITY;
    int samples = m_maxSamples - m_quality * factor;
    return samples;
}


std::string Lock::toColorString(LockState state)
{
    switch (state)
    {
    case UNLOCKED : return RED "unlocked" RESET;
    case LOCKED : return DARKGREEN "locked" RESET;
    case HILOCK : return GREEN "high lock" RESET;
    }
    return "";
}


std::string Lock::toString(LockState state)
{
    switch (state)
    {
    case UNLOCKED : return "unlocked";
    case LOCKED : return "locked";
    case HILOCK : return "high lock";
    }
    return "";
}


Lock::LockState Lock::errorOffset(double offset)
{
    LockState lockState = m_lockState;
    const int LOCK_COUNTS = 3;

    if (std::fabs(offset) < UNLOCK_THRESHOLD)
    {
        if (m_counter < LOCK_COUNTS)
        {
            m_lockState = UNLOCKED;
            ++m_counter;
        }
        else if (std::fabs(offset) < HILOCK_THRESHOLD)
        {
            if (++m_counter >= LOCK_COUNTS + 2)
            {
                m_lockState = HILOCK;
                ++m_quality;
            }
            else
            {
                m_lockState = LOCKED;
            }
        }
        else if (std::fabs(offset) < STDLOCK_THRESHOLD)
        {
            m_counter = LOCK_COUNTS;
            m_quality /= 2;
            m_lockState = LOCKED;
        }
        else
        {
            m_counter = LOCK_COUNTS;
            m_quality /= 4;
            m_lockState = LOCKED;
        }

        if (m_quality < 0)
        {
            m_quality = 0;
        }
        else if (m_quality > MAX_QUALITY)
        {
            m_quality = MAX_QUALITY;
        }
    }
    else
    {
        m_lockState = UNLOCKED;
        m_counter = 0;
        m_quality = 0;
    }

    if (lockState != m_lockState)
    {
        trace->info("[{}] lock status changed from {} to {}", m_clientName, toColorString(lockState), toColorString(m_lockState));
        emit signalNewLockState(m_lockState);
    }
    return m_lockState;
}


void Lock::panic()
{
    if (m_quality > 1)
    {
        m_quality = 1;
    }
    if (m_lockState >= LOCKED)
    {
        trace->info("[{}] lock status changed from {} to {}", m_clientName, toColorString(m_lockState), toColorString(LOCKED));
        m_lockState = LOCKED;
    }
}
