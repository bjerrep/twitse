#pragma once

#include "basicoffsetmeasurement.h"

#include <deque>

using OffsetMeasurementVector = std::vector<OffsetMeasurement>;

class OffsetMeasurementHistory
{
public:
    OffsetMeasurementHistory(double maxSeconds = 3600.0, int maxMeasurements = 100);

    void add(OffsetMeasurement sum);

    void reset();
    void setFlags(DevelopmentMask develMask);

    double getPPM() const;
    double getMovingAveragePPM() const;
    double getLastTimespan_sec() const;
    double getSD_us() const;
    double getMeanAbsoluteDeviation_us() const;

    std::string clientToString(uint16_t dac) const;
    OffsetMeasurementVector getMeasurementsSummary() const;
    int getCounter() const;
    int size() const;

private:
    void update();
    double getTimeSpan_sec() const;

private:
    OffsetMeasurementVector m_offsetMeasurements;
    OffsetMeasurementVector m_offsetMeasurementsSummary;
    std::deque<double> m_movingAverageSlope;

    double m_minSeconds;
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

    friend class DataAnalyse;
};
