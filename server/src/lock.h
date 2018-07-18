#pragma once
#include "globals.h"

#include <string>
#include <QObject>


class Lock : public QObject
{
    Q_OBJECT

    const double UNLOCK_THRESHOLD = 50.0;
    const double HILOCK_THRESHOLD = 20.0;
    const double STDLOCK_THRESHOLD = 30.0;
    const int MAX_DELAY_SECS = 60;
    const int MAX_QUALITY = 20;

public:
    enum LockState
    {
        UNLOCKED,
        LOCKED,
        HILOCK
    };

    enum Distribution
    {
        BURST_SILENCE,
        EVENLY_DISTRIBUTED
    };

    Lock(const std::string& clientname);

    bool isHiLock() const;
    bool isLock() const;
    LockState getLockState() const;
    int getInterMeasurementDelaySecs() const;
    int getPeriodMSecs() const;
    int getMeasurementPeriodsecs() const;
    int getNofSamples() const;
    static std::string toColorString(LockState state);
    static std::string toString(LockState state);
    LockState errorOffset(double offset);
    void panic();

signals:
    void signalNewLockState(LockState m_lockState);

private:
    LockState m_lockState = UNLOCKED;
    std::string m_clientName;
    int m_counter = 0;
    int m_quality = 0;
    int m_maxSamples = g_maxNofSamples;
    Distribution m_distribution = BURST_SILENCE;
};
