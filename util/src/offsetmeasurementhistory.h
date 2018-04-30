#pragma once

#include "basicoffsetmeasurement.h"

#include <deque>

using OffsetMeasurementVector = std::vector<OffsetMeasurement>;

class OffsetMeasurementHistory
{
public:
    const int LENGTH = 20;

    OffsetMeasurementHistory(double maxSeconds = 60.0, int maxMeasurements = 5);

    double add(OffsetMeasurement sum);

    void reset();
    void setFlags(DevelopmentMask develMask);
    void enableAverages();

    double getPPM() const;
    double getMovingAveragePPM() const;
    double getLastTimespan_sec() const;
    double getSD_us() const;

    std::string toString() const;
    OffsetMeasurementVector getMeasurementsSummary() const;
    int getCounter() const;

private:
    void update();
    double getTimeSpan_sec() const;
    int nofMeasurements() const;

private:
    OffsetMeasurementVector m_offsetMeasurements;
    OffsetMeasurementVector m_offsetMeasurementsSummary;
    std::deque<double> m_movingAverageSlope;

    double m_maxSeconds;
    int m_minMeasurements;
    DevelopmentMask m_develMask = DevelopmentMask::None;
    double m_slope = 0.0;
    double m_sd_ns = -1.0;
    int m_loop = 0;

    double m_averageSlope = 0.0;
    double m_averageOffset_ns = 0.0;
    double m_movingAverageOffset_ns = 0.0;
    bool m_initialize = true;
    int m_totalMeasurements = 0;
    bool m_averagesEnabled = false;

    friend class DataAnalyse;
};
