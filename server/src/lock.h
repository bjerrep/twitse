#pragma once
#include "globals.h"

#include <string>
#include <QObject>


class Lock : public QObject
{
    Q_OBJECT

    /* A quality level determines the number of samples in a measurement, and the spacing
     * between measurements (see below)
     */
    const static int QUALITY_LEVELS = 11;  // 0 (worst) - 10 (best)

    /* The very central value of the time spacing between samples. There must be
     * a deeper connection to e.g. the workings of the air timing in wifi but with
     * no synchonisation in place the actuel value seems to change pretty much nothing
     * with regard to roundtrip quality.
     * It will however have an effect on all the hardcoded constants elsewhere in the code...
     */
#ifdef VCTCXO
    const int BURST_SAMPLE_RATE_ms = 10;
#else
    const int BURST_SAMPLE_RATE_ms = 10;
#endif


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

    Lock(std::string clientname);

    bool isLocked() const;
    LockState getLockState() const;
    int getInterMeasurementDelaySecs() const;
    int getSamplePeriod_ms() const;
    int getMeasurementBurstSleepPeriod_sec() const;
    int getMeasurementDuration_ms() const;
    int getNofSamples() const;
    int getQuality() const;
    int getQualityPct(int quality = -1) const;
    Distribution getDistribution() const;
    LockState updateLockState(double offset);
    void panic();

    static std::string toColorString(LockState state);
    static std::string toString(LockState state);
    static void setFixedMeasurementSilence_sec(int period);
    static void setFixedClientSamples(int samples);
    void setFixedSamplePeriod_ms(int ms);

signals:
    void signalNewLockAndQuality();

private:
    LockState m_lockState = UNLOCKED;
    std::string m_clientName;
    int m_counter = 0;
    int m_quality = 0;
    Distribution m_distribution = BURST_SILENCE;

    // these might get set when server is starting before there is any lock objects
    static int s_fixedMeasurementSilence_sec;
    static int s_clientSamples;
    int m_fixedSamplePeriod_ms = -1;

    const int Samples[QUALITY_LEVELS] = {500, 450, 400, 360, 330, 290, 255, 235, 220, 210, 200};
    const int Seconds[QUALITY_LEVELS] = {  5,   7,  10,  15,  22,  32,  45,  60,  75,  90, 110};
};
