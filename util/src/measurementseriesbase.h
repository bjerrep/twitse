#pragma once

#include "offsetmeasurement.h"
#include "globals.h"
#include <vector>


class MeasurementSeriesBase
{
public:

    enum FilterType
    {
        DEFAULT,
        EVERYTHING,    // development only in order to keep all samples for analysis
        LOWEST_VALUES, // least sophisticated but hard to beat.
        LARGEST_BIN_WINDOW  // default for VCTCXO, not tested in software mode
    };

    const std::vector<std::string> FilterAsString = {
        "default", "everything", "lowest values", "largest bin window"};


    virtual ~MeasurementSeriesBase() {}

    virtual void add(int64_t rawserverTime, int64_t rawclientTime) = 0;

    virtual void prepareNewDataMeasurement(int samples = 0) = 0;

    virtual OffsetMeasurement calculate() = 0;

    virtual void setFiltering(FilterType filterType) = 0;

    virtual void saveRawMeasurements(std::string filename, int serial) const = 0;

    virtual void saveFilteredMeasurements(std::string filename, int serial) const = 0;
};
