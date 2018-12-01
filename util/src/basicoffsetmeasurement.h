#pragma once

#include "mathfunc.h"
#include "measurementseriesbase.h"


class BasicMeasurementSeries : public MeasurementSeriesBase
{
    const std::vector<std::string> FilterAsString = {
        "default", "everything", "lowest values", "tracking", "histogram"};

public:
    BasicMeasurementSeries(const std::string &logName, FilterType filterType = DEFAULT);

    void add(int64_t rawserverTime, int64_t rawclientTime) override;
    void prepareNewDataMeasurement(int samples) override;
    OffsetMeasurement calculate() override;
    void setFiltering(BasicMeasurementSeries::FilterType filterType) override;
    void saveRawMeasurements(std::string filename, int serial) const override;

private:
    SampleList64 getTrackingVector(const SampleList64 &rawvalues) const;

    bool filterMeasurementsInRange(const SampleList64 &diff,
                            SampleList64 &filtered_time, SampleList64 &filtered_diff,
                            int64_t lowerLimit, int64_t upperLimit,
                            int64_t &diff_average);

    bool filterMeasurementsAroundVector(const SampleList64 &diff, const SampleList64 &track,
                                        int64_t lower_limit, int64_t upper_limit,
                                        SampleList64 &filtered_time, SampleList64 &newdiff,
                                        int64_t &diff_average);

    bool accept(const SampleList64 &newtime,
                const SampleList64 &newdiff,
                const SampleList64 &localTime,
                const SampleList64 &remoteTime,
                const SampleList64 &diff);

    std::string m_logName;
    SampleList64 m_remoteTime;
    SampleList64 m_localTime;

    FilterType m_filterType;

    int m_nofSeries = 1;
    int m_measurementRun = 0;
    int m_samples = 0;
};

