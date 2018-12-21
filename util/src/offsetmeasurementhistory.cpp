#include "offsetmeasurementhistory.h"
#include "mathfunc.h"
#include "log.h"
#include "systemtime.h"

#include <numeric>
#include <cmath>

OffsetMeasurementHistory::OffsetMeasurementHistory(double minSeconds, int minMeasurements)
    : m_minSeconds(minSeconds),
      m_minMeasurements(minMeasurements)
{
}


void OffsetMeasurementHistory::setFlags(DevelopmentMask develMask)
{
    m_develMask = develMask;
}


int OffsetMeasurementHistory::getCounter() const
{
    return m_loop;
}


void OffsetMeasurementHistory::reset()
{
    m_offsetMeasurements.clear();
    m_movingAverageSlope.clear();
    m_loop = 0;
    m_slope = 0.0;
    m_averageSlope = 0.0;
}


void OffsetMeasurementHistory::add(OffsetMeasurement sum)
{
    m_loop++;
    m_offsetMeasurements.push_back(sum);
    if (m_develMask & DevelopmentMask::AnalysisAppendToSummary)
    {
        sum.m_ppm = getPPM();
        m_offsetMeasurementsSummary.push_back(sum);
    }

    while ((int) m_offsetMeasurements.size() > m_minMeasurements && getTimeSpan_sec() > m_minSeconds)
    {
        m_offsetMeasurements.erase(m_offsetMeasurements.begin());
    }

    update();
}


void OffsetMeasurementHistory::update()
{
    SampleList64 m_time;
    SampleList64 m_offset;

    m_totalMeasurements = 0;

    for(const OffsetMeasurement& summary : m_offsetMeasurements)
    {
        m_time.push_back(summary.m_endtime_ns);
        m_offset.push_back(summary.m_offset_ns);
        m_totalMeasurements += summary.m_collectedSamples;
    }

    m_averageOffset_ns = MathFunc::average(m_offset);

    if (size() < 2)
    {
        return;
    }

    double constant;
    MathFunc::linearRegression(m_time, m_offset, m_slope, constant);

    OffsetMeasurement& last_measurement = m_offsetMeasurements.back();

    {
        m_movingAverageSlope.push_back(m_slope);

        if (m_movingAverageSlope.size() > 1)
        {
            if (m_movingAverageSlope.size() > 5)
            {
                m_movingAverageSlope.pop_front();
            }
            m_averageSlope = std::accumulate(m_movingAverageSlope.begin(), m_movingAverageSlope.end(), 0.0) / m_movingAverageSlope.size();

            if (m_initialize)
            {
                if (!--m_initialize)
                {
                    m_movingAverageOffset_ns = last_measurement.m_offset_ns;
                }
                else
                {
                    m_movingAverageOffset_ns = 0.0;
                }
            }
            else
            {
                m_movingAverageOffset_ns = m_movingAverageOffset_ns * 0.9 + last_measurement.m_offset_ns * 0.1;
            }

            m_sd_ns = MathFunc::standardDeviation(m_time, m_offset, m_slope);
        }
    }
}


double OffsetMeasurementHistory::getPPM() const
{
    return m_slope * 1000000.0;
}


double OffsetMeasurementHistory::getMovingAveragePPM() const
{
    return m_averageSlope * 1000000.0;
}


double OffsetMeasurementHistory::getLastTimespan_sec() const
{
    try
    {
        return (m_offsetMeasurements.at(size() - 1).m_endtime_ns -
                m_offsetMeasurements.at(size() - 2).m_endtime_ns)
                / NS_IN_SEC_F;
    }
    catch (const std::out_of_range&)
    {
        trace->error("getLastTimespan_sec index out of range");
        return 0.0;
    }
}


double OffsetMeasurementHistory::getSD_us() const
{
    SampleList64 offsets;
    for(OffsetMeasurement offsetMeasurement : m_offsetMeasurements)
    {
        offsets.push_back(offsetMeasurement.m_clientOffset_ns);
    }
    return MathFunc::standardDeviation(offsets) / 1000.0;
}


double OffsetMeasurementHistory::getMeanAbsoluteDeviation_us() const
{
    if (!size())
    {
        return 0.0;
    }
    double mad = 0.0;
    for(OffsetMeasurement offsetMeasurement : m_offsetMeasurements)
    {
        mad += std::fabs(offsetMeasurement.m_clientOffset_ns);
    }
    return mad / (size() * 1000.0);
}


double OffsetMeasurementHistory::getTimeSpan_sec() const
{
    return (m_offsetMeasurements.back().m_endtime_ns - m_offsetMeasurements.front().m_endtime_ns) /
            NS_IN_SEC_F;
}


size_t OffsetMeasurementHistory::size() const
{
    return m_offsetMeasurements.size();
}


OffsetMeasurementVector OffsetMeasurementHistory::getMeasurementsSummary() const
{
    return m_offsetMeasurementsSummary;
}


std::string OffsetMeasurementHistory::clientToString(uint16_t dac) const
{
    OffsetMeasurement const& last_measurement = m_offsetMeasurements.back();
    double offset_us = last_measurement.m_offset_ns / 1000.0;
    double package_loss = 100.0 * last_measurement.m_usedSamples / last_measurement.m_collectedSamples;

    if (VCTCXO_MODE)
    {
        return fmt::format("{:-3d} runtime {:5.1f} samples/used {}/{:4.1f}% secs/size {:5.1f}/{} "
                           "offset_us {:-5.1f} avgoff_us {:-5.1f}"
                           " ppm {:-7.3f} avgppm {:-7.3f} sd_us {:-5.1f} vctcxo {}",
                           m_loop, s_systemTime->getRunningTime_secs(), m_totalMeasurements, package_loss,
                           getTimeSpan_sec(), m_offsetMeasurements.size(),
                           offset_us, m_movingAverageOffset_ns/1000.0,
                           getPPM(), m_averageSlope * 1000000.0, m_sd_ns/1000.0,
                           dac);
    }

    return fmt::format("{:-3d} runtime {:5.1f} samples/used {}/{:4.1f}% secs/size {:5.1f}/{} "
                       "offset_us {:-5.1f} avgoff_us {:-5.1f}"
                       " ppm {:-7.3f} avgppm {:-7.3f} sd_us {:-5.1f} systimeppm {:-7.3f}",
                       m_loop, s_systemTime->getRunningTime_secs(), m_totalMeasurements, package_loss,
                       getTimeSpan_sec(), m_offsetMeasurements.size(),
                       offset_us, m_movingAverageOffset_ns/1000.0,
                       getPPM(), m_averageSlope * 1000000.0, m_sd_ns/1000.0,
                       s_systemTime->getPPM());
}
