#pragma once

#include "mathfunc.h"
#include "measurementseriesbase.h"


class BasicMeasurementSeries : public MeasurementSeriesBase
{
    enum FilterType
    {
        EVERYTHING,
        LOWEST_VALUES,
        TRACKING_RANGE_VALUES
    };

    const std::vector<std::string> FilterAsString = {"everything", "lowest values", "tracking"};

public:
    BasicMeasurementSeries(const std::string &logName = "");

    void add(int64_t rawserverTime, int64_t rawclientTime);
    void prepareNewDataMeasurement(int samples);
    OffsetMeasurement calculate();
    void setFiltering(BasicMeasurementSeries::FilterType filterType);

private:
    SampleList64 getTrackingVector(const SampleList64 &rawvalues) const;

    bool filterMeasurements(const SampleList64 &diff,
                            SampleList64 &filtered_time, SampleList64 &filtered_diff,
                            int64_t lowerLimit, int64_t upperLimit,
                            int64_t &diff_average);

    bool filterMeasurementsAroundVector(const SampleList64 &diff, const SampleList64 &track,
                                        int64_t lower_limit, int64_t upper_limit,
                                        SampleList64 &filtered_time, SampleList64 &newdiff,
                                        int64_t &diff_average);

    bool accept(const SampleList64 &newtime, const SampleList64 &newdiff);

    std::string m_logName;
    SampleList64 m_remoteTime;
    SampleList64 m_localTime;

    FilterType m_filterType = LOWEST_VALUES;

    int m_nofSeries = 1;
    int m_nofMeasurements = 0;
    int m_samples = 0;
};

