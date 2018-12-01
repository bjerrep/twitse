#pragma once

#include <stdint.h>
#include <string>

/// The calculation result from a measurement series most importantly
/// containing the net average calculated offset.
class OffsetMeasurement
{
public:
    OffsetMeasurement(int index, int64_t starttime_ns, int64_t endtime_ns,
                      int samples_sent, int samples, size_t used, int64_t avg_ns, bool valid);

    void setOffset_ns(int64_t offset_ns);
    std::string toString() const;
    double packageLossPct() const;

public:
    int m_index;
    int64_t m_starttime_ns;
    int64_t m_endtime_ns;
    int m_sentSamples;
    int m_collectedSamples;
    int m_usedSamples;
    int64_t m_offset_ns;
    bool m_valid = true;
    int64_t m_clientOffset_ns = 0;

    // does not really belong here but the current client ppm needed a home
    // for analysis table dumps
    double m_ppm = 0.0;
};
