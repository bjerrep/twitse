#pragma once

#include "offsetmeasurement.h"
#include "globals.h"


class MeasurementSeriesBase
{
public:
    virtual ~MeasurementSeriesBase() {}

    virtual void add(int64_t rawserverTime, int64_t rawclientTime) = 0;

    virtual void prepareNewDataMeasurement(int samples = 0) = 0;

    virtual OffsetMeasurement calculate() = 0;
};
