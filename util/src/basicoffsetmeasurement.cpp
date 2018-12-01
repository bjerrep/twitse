#include "basicoffsetmeasurement.h"
#include "log.h"
#include "spdlog/fmt/fmt.h"
#include "datafiles.h"

#include <QIODevice>
#include <QFile>
#include <QTextStream>

#include <cmath>
#include <algorithm>
#include <ctime>

extern DevelopmentMask g_developmentMask;

BasicMeasurementSeries::BasicMeasurementSeries(const std::string& logName, FilterType filterType)
    : m_logName(logName),
      m_filterType(filterType)
{
    if (m_filterType == DEFAULT)
    {
        m_filterType = VCTCXO_MODE ? MeasurementSeriesBase::HISTOGRAM : MeasurementSeriesBase::LOWEST_VALUES;
    }
    trace->info("{} sample filtering algorithm is '{}'", m_logName, FilterAsString[m_filterType]);
}


void BasicMeasurementSeries::add(int64_t remoteTime, int64_t localTime)
{
    m_remoteTime.push_back(remoteTime);
    m_localTime.push_back(localTime);
    m_measurementRun++;
}


void BasicMeasurementSeries::prepareNewDataMeasurement(int samples)
{
    m_remoteTime.clear();
    m_localTime.clear();
    m_nofSeries++;
    m_samples = samples;
}


bool BasicMeasurementSeries::accept(const SampleList64& newtime,
                                    const SampleList64& newdiff,
                                    const SampleList64& localTime,
                                    const SampleList64& remoteTime,
                                    const SampleList64& diff)
{
    double lost_pct = m_samples ? 100.0 - 100.0 * m_localTime.size() / m_samples : 0.0;
    if (lost_pct > 50.0)
    {
        trace->debug("{}package loss is {:.1f}%, bailing out", m_logName, lost_pct);
        return false;
    }
    if (lost_pct > 10.0)
    {
        trace->warn("{}package loss is {:.1f}%", m_logName, lost_pct);
    }

    double removed_pct = (100.0 * (m_localTime.size() - newdiff.size())) / m_localTime.size();
    if (removed_pct > 80.0)
    {
        trace->warn("{}filtered out {:.1f}% samples, bailing out", m_logName, removed_pct);
        if (g_developmentMask & DevelopmentMask::OnBailingOut)
        {
            DataFiles::dumpVectors("onbailingout_raw", m_nofSeries, &localTime, &remoteTime, &diff);
            DataFiles::dumpVectors("onbailingout_filtered", m_nofSeries, &newtime, &newdiff);
        }
        return false;
    }
    if (removed_pct > 50.0)
    {
        trace->warn("{}filtered out {:.1f}% samples", m_logName, removed_pct);
    }

    return true;
}


bool BasicMeasurementSeries::filterMeasurementsInRange(
        const SampleList64& diff,
        SampleList64 &filtered_time,
        SampleList64 &filtered_diff,
        int64_t lower_limit,
        int64_t upper_limit,
        int64_t &diff_average)
{
    if (m_localTime.empty())
    {
        trace->error("{}fatal error in filterMeasurements: no data recieved", m_logName);
        return false;
    }

    for (uint i=0; i< m_localTime.size(); i++)
    {
        if ( diff.at(i) >= lower_limit && diff.at(i) <= upper_limit)
        {
            filtered_diff.push_back(diff.at(i));
            filtered_time.push_back(m_localTime.at(i));
        }
    }
    diff_average = MathFunc::average(filtered_diff);
    return true;
}


bool BasicMeasurementSeries::filterMeasurementsAroundVector(
        const SampleList64& diff,
        const SampleList64& track,
        int64_t lower_limit, int64_t upper_limit,
        SampleList64 &filtered_time, SampleList64 &filtered_diff,
        int64_t &diff_average)
{
    if (track.empty())
    {
        trace->error("{}fatal error in filterMeasurements: no data recieved", m_logName);
        return false;
    }

    for (uint i=0; i< m_localTime.size(); i++)
    {
        int64_t track_diff = diff[i] - track[i];
        if ( track_diff >= lower_limit && track_diff <= upper_limit)
        {
            filtered_diff.push_back(diff.at(i));
            filtered_time.push_back(m_localTime.at(i));
        }
    }
    diff_average = MathFunc::average(filtered_diff);
    return true;
}


SampleList64 BasicMeasurementSeries::getTrackingVector(const SampleList64& rawvalues) const
{
    double step_ns = 10000.0;
    SampleList64 track;

    double average = MathFunc::average(rawvalues);

    for (int i = rawvalues.size() - 2; i >= 0; --i)
    {
        if (rawvalues.at(i) > average)
            average += step_ns;
        else
            average -= step_ns;
    }

    step_ns = 5000.0;

    for (const auto& value : rawvalues)
    {
        if (value > average)
            average += step_ns;
        else
            average -= step_ns;
        track.push_back(average);
    }

    if (g_developmentMask & DevelopmentMask::SaveMeasurements)
    {
        DataFiles::dumpVectors("raw_tracking_vector", m_nofSeries, &track);
    }

    return track;
}


OffsetMeasurement BasicMeasurementSeries::calculate()
{
    // produce the difference vector between local time vs remote time
    // ----------------------------------------------------------------
    // a positive number means that the local time is ahead of the remote time
    // and the ppm calculated elsewhere will be positive.

    int incomming_samples = m_remoteTime.size();
    SampleList64 diff(incomming_samples);

    std::transform(m_localTime.begin(), m_localTime.end(),
                   m_remoteTime.begin(),
                   diff.begin(),
                   std::minus<int64_t>());

    if (g_developmentMask & DevelopmentMask::SaveMeasurements)
    {
        DataFiles::dumpVectors("raw_clienttime_and_serverdelay", m_nofSeries, &m_localTime, &diff);
    }

    // sanitize the measurements. Remove the worst outliers
    // ----------------------------------------------------

    SampleList64 filtered_time, filtered_diff;
    int64_t average_offset_ns = 0.0;
    bool success = true;
    bool standard_accept_check = true;
    size_t filtered_samples = 0;

    switch (m_filterType)
    {
    case DEFAULT:
    {
        trace->critical("can't apply default filter..");
        success = false;
        break;
    }
    case EVERYTHING:
    {
        filtered_samples = diff.size();
        average_offset_ns = MathFunc::average(diff);
        break;
    }
    case LOWEST_VALUES:
    {
        const int range = 600000;
        int64_t minimum = *std::min_element(std::begin(diff), std::end(diff));
        int64_t maximum = minimum + range;
        success = filterMeasurementsInRange(diff,
                                     filtered_time, filtered_diff,
                                     minimum, maximum,
                                     average_offset_ns);
        filtered_samples = filtered_time.size();
        break;
    }
    case TRACKING_RANGE_VALUES:
    {
        SampleList64 track = getTrackingVector(diff);
        success = filterMeasurementsAroundVector(diff, track,
                                                 -300000, 400000,
                                                 filtered_time, filtered_diff,
                                                 average_offset_ns);
        filtered_samples = filtered_time.size();
        break;
    }
    case HISTOGRAM:
    {
        standard_accept_check = false;
        average_offset_ns = 0.0;
        filtered_samples = 0;

        MathMeasurement selection;
        if (success)
        {
            MathMeasurement meas;
            meas.copy(m_localTime, m_remoteTime);
            MathMeasurementHistogram bin(100, meas);
            success = bin.getCentralPercentage(25.0, selection);
        }
        if (success)
        {
            average_offset_ns = selection.averageOffset_ns();
            filtered_samples = selection.size();
        }
    }
    }

    if (success && standard_accept_check)
    {
        success = accept(filtered_time, filtered_diff, m_localTime, m_remoteTime, diff);
    }

    OffsetMeasurement offsetMeasurement(m_nofSeries,
                                        m_localTime.front(), m_localTime.back(),
                                        m_measurementRun,
                                        incomming_samples,
                                        filtered_samples,
                                        average_offset_ns,
                                        success);

    return offsetMeasurement;
}

// development
void BasicMeasurementSeries::setFiltering(BasicMeasurementSeries::FilterType filterType)
{
    m_filterType = filterType;
}


void BasicMeasurementSeries::saveRawMeasurements(std::string filename, int serial) const
{
    SampleList64 diff(m_localTime.size());

    std::transform(m_localTime.begin(), m_localTime.end(),
                   m_remoteTime.begin(),
                   diff.begin(),
                   std::minus<int64_t>());
    DataFiles::dumpVectors(filename.c_str(), serial, &m_localTime, &m_remoteTime, &diff);
}
