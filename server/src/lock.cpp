#include "lock.h"
#include "log.h"

extern int g_developmentMask;

int Lock::s_fixedMeasurementSilence_sec = -1;
int Lock::s_clientSamples = -1;


Lock::Lock(std::string clientname)
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
        if (s_fixedMeasurementSilence_sec >= 0)
        {
            return s_fixedMeasurementSilence_sec;
        }
        return std::max(Seconds[m_quality] - Samples[m_quality] * getSamplePeriod_ms() / 1000.0, 0.0);
    }
    return 0;
}


int Lock::getSamplePeriod_ms() const
{
    if (m_fixedSamplePeriod_ms >= 0)
    {
        return m_fixedSamplePeriod_ms;
    }

    switch (m_distribution)
    {
    case EVENLY_DISTRIBUTED:
        //return 1000 * getMeasurementPeriod_sec() / getNofSamples();
        return (1000 * Seconds[m_quality]) / Samples[m_quality];
    case BURST_SILENCE:
        return MIN_SAMPLE_INTERVAL_ms;
    }
    return 0;
}


void Lock::setFixedMeasurementSilence_sec(int period)
{
    s_fixedMeasurementSilence_sec = period;
}


void Lock::setFixedClientSamples(int samples)
{
    s_clientSamples = samples;
}


// development
void Lock::setFixedSamplePeriod_ms(int ms)
{
    m_fixedSamplePeriod_ms = ms;
    m_distribution = EVENLY_DISTRIBUTED;
}


int Lock::getMeasurementPeriod_sec() const
{
    return Seconds[m_quality];
}


int Lock::getNofSamples() const
{
    if (s_clientSamples >= 0)
        return s_clientSamples;

    return Samples[m_quality];
}


Lock::Distribution Lock::getDistribution() const
{
    return m_distribution;
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


Lock::LockState Lock::update(double offset)
{
    LockState lockState = m_lockState;
    const int LOCK_COUNTS = 3;

    const double UNLOCK_THRESHOLD = 50.0;
    const double STDLOCK_THRESHOLD = 30.0;
    const double HILOCK_THRESHOLD = 20.0;
    const double HILOCK_SAFE_THRESHOLD = 15.0;

    offset = std::fabs(offset);

    if (offset < UNLOCK_THRESHOLD)
    {
        if (m_counter < LOCK_COUNTS)
        {
            m_lockState = UNLOCKED;
            ++m_counter;
        }
        else if (offset < HILOCK_THRESHOLD)
        {
            if (++m_counter >= LOCK_COUNTS + 2)
            {
                m_lockState = HILOCK;
                if (offset > HILOCK_SAFE_THRESHOLD)
                {
                    m_quality--;
                }
                else
                {
                    m_quality++;
                }
            }
            else
            {
                m_lockState = LOCKED;
            }
        }
        else if (offset < STDLOCK_THRESHOLD)
        {
            m_counter = LOCK_COUNTS;
            m_quality -= 2;
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
        else if (m_quality >= QUALITY_LEVELS)
        {
            m_quality = QUALITY_LEVELS - 1;
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
