#pragma once

#include <stdint.h>
#include <string>

/// The calculation result from a measurement series most importantly
/// containing the net average calculated offset.
class OffsetMeasurement
{
public:
    enum ResultCode
    {
        PASS,
        INTERNALERROR,
        NO_DATA,
        EXCESSIVE_PACKAGELOSS,
        FILTER_ERROR
    };

    static std::string ResultCodeAsString(ResultCode resultCode)
    {
        static std::string resultStrings[] =
        {
            "Pass",
            "Internal error",
            "No data",
            "Package loss",
            "Filtered too much"
        };
        return resultStrings[static_cast<int>(resultCode)];
    }

public:
    OffsetMeasurement(int index, int64_t starttime_ns, int64_t endtime_ns,
                      size_t samples_sent, size_t samples, size_t used, int64_t avg_ns, ResultCode resultCode);

    void setOffset_ns(int64_t offset_ns);
    std::string toString() const;
    double packageLossPct() const;
    ResultCode resultCode() const;

public:
    int m_index;
    int64_t m_starttime_ns;
    int64_t m_endtime_ns;
    size_t m_sentSamples;
    size_t m_collectedSamples;
    size_t m_usedSamples;
    int64_t m_offset_ns;
    int64_t m_clientOffset_ns = 0;
    ResultCode m_resultCode = ResultCode::PASS;

    // does not really belong here but the current client ppm needed a home
    // for analysis table dumps
    double m_ppm = 0.0;

};
