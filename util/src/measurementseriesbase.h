#pragma once

#include "offsetmeasurement.h"
#include "globals.h"


class MeasurementSeriesBase
{
public:
    enum FilterType
    {
        DEFAULT,
        EVERYTHING, // development only in order to keep all samples
        LOWEST_VALUES, // least sophisticated
        TRACKING_RANGE_VALUES,
        HISTOGRAM
    };

    virtual ~MeasurementSeriesBase() {}

    virtual void add(int64_t rawserverTime, int64_t rawclientTime) = 0;

    virtual void prepareNewDataMeasurement(int samples = 0) = 0;

    virtual OffsetMeasurement calculate() = 0;

    virtual void setFiltering(FilterType filterType) = 0;

    virtual void saveRawMeasurements(std::string filename, int serial) const = 0;
};
