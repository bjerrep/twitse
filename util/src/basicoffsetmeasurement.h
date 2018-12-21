#pragma once

#include "mathfunc.h"
#include "measurementseriesbase.h"


class BasicMeasurementSeries : public MeasurementSeriesBase
{
public:
    BasicMeasurementSeries(const std::string &logName, FilterType filterType = DEFAULT);

    void add(int64_t rawserverTime, int64_t rawclientTime) override;
    void prepareNewDataMeasurement(int samples) override;
    OffsetMeasurement calculate() override;
    void setFiltering(BasicMeasurementSeries::FilterType filterType) override;

    void saveRawMeasurements(std::string filename, int serial) const override;
    void saveFilteredMeasurements(std::string filename, int serial) const override;

private:
    bool filterMeasurementsInRange(const SampleList64 &time, const SampleList64 &diff,
            SampleList64 &filtered_time, SampleList64 &filtered_diff,
            int64_t lowerLimit, int64_t upperLimit,
            int64_t &average);

    int64_t averageFromLargestHistogramBin(const SampleList64 &_x,
                                           size_t nofBins,
                                           int histogramRange_ns);

    bool accept(size_t receivedSamples,
                size_t filteredSamples,
                double lossFailed,
                double lossWarn,
                double filteredFailed, double filteredWarn);

    std::string m_logName;
    SampleList64 m_remoteTime;
    SampleList64 m_localTime;

    SampleList64 filtered_time, filtered_diff;

    FilterType m_filterType;

    int m_nofSeries = 1;
    size_t m_measurementRun = 0;
    int m_samples = 0;
};

