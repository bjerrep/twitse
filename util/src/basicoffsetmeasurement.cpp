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

extern int g_developmentMask;


BasicMeasurementSeries::BasicMeasurementSeries(std::string logName, FilterType filterType)
    : m_logName(logName),
      m_filterType(filterType)
{
    if (m_filterType == DEFAULT)
    {
        m_filterType = VCTCXO_MODE ? MeasurementSeriesBase::LARGEST_BIN_WINDOW : MeasurementSeriesBase::LARGEST_BIN_WINDOW;
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


OffsetMeasurement::ResultCode BasicMeasurementSeries::accept(
        size_t receivedSamples,
        size_t filteredSamples,
        double lossFailed, double lossWarn,
        double filteredFailed, double filteredWarn)
{
    double lost_pct = m_samples ? 100.0 - 100.0 * receivedSamples / m_samples : 0.0;
    if (lost_pct > lossFailed)
    {
        trace->warn("{}package loss is {:.1f}%, bailing out", m_logName, lost_pct);
        return OffsetMeasurement::EXCESSIVE_PACKAGELOSS;
    }
    if (lost_pct > lossWarn)
    {
        trace->warn("{}package loss is {:.1f}%", m_logName, lost_pct);
    }

    double removed_pct = (100.0 * (receivedSamples - filteredSamples)) / receivedSamples;
    if (removed_pct > filteredFailed)
    {
        trace->warn("{}filtered out {:.1f}% samples, bailing out", m_logName, removed_pct);
        return OffsetMeasurement::FILTER_ERROR;
    }
    if (removed_pct > filteredWarn)
    {
        trace->warn("{}filtered out {:.1f}% samples", m_logName, removed_pct);
    }

    return OffsetMeasurement::PASS;
}


OffsetMeasurement::ResultCode BasicMeasurementSeries::filterMeasurementsInRange(
        const SampleList64& time,
        const SampleList64& diff,
        SampleList64 &filtered_time,
        SampleList64 &filtered_diff,
        int64_t lower_limit,
        int64_t upper_limit,
        int64_t &average)
{
    if (time.empty())
    {
        trace->error("{}fatal error in filterMeasurements: no data recieved", m_logName);
        return OffsetMeasurement::NO_DATA;
    }

    for (size_t i = 0; i < time.size(); i++)
    {
        if (diff.at(i) >= lower_limit && diff.at(i) <= upper_limit)
        {
            filtered_diff.push_back(diff.at(i));
            filtered_time.push_back(time.at(i));
        }
    }
    average = MathFunc::average(filtered_diff);
    return OffsetMeasurement::PASS;
}


int64_t BasicMeasurementSeries::averageFromLargestHistogramBin(const SampleList64 &_x,
                                                               size_t nofBins,
                                                               int histogramRange_ns)
{
    SampleList32 m_bins;
    m_bins.assign(nofBins, 0);

    int64_t bin_width_ns = histogramRange_ns / nofBins;
    int64_t lower = MathFunc::min(_x);
    int largest_value = 0;
    uint largest_index = 0;
    for (auto x : _x)
    {
        uint index = (x - lower) / bin_width_ns;
        if (index < nofBins)
        {
            m_bins[index]++;
            if (m_bins[index] > largest_value)
            {
                largest_index = index;
                largest_value = m_bins[index];
            }
            else if (m_bins[index] == largest_value and index < largest_index)
            {
                largest_index = index;
            }
        }
    }

    if (largest_value <= 4)
    {
        trace->warn("largest histogram average bin contains only {} samples", largest_value);
    }

    double correction = 0.0;
    if (largest_value > 0 and largest_index > 0 and largest_index < m_bins.size() - 1)
    {
        double lo_loss = (largest_value - m_bins[largest_index-1]) / ((double) largest_value);
        double hi_loss = (largest_value - m_bins[largest_index+1]) / ((double) largest_value);
        correction = (lo_loss - hi_loss) / 2.0;
    }

    return lower + largest_index * bin_width_ns + bin_width_ns / 2 + correction * bin_width_ns;
}


OffsetMeasurement BasicMeasurementSeries::calculate()
{
    // produce the difference vector between local time vs remote time
    // ----------------------------------------------------------------
    // a positive number means that the local time is ahead of the remote time
    // and the ppm calculated elsewhere will be positive.

    SampleList64 diff = MathFunc::diff(m_localTime, m_remoteTime);

    if (g_developmentMask & DevelopmentMask::SaveMeasurements or
            g_developmentMask & DevelopmentMask::SaveMeasurementsSingle)
    {
        DataFiles::dumpVectors("/tmp/raw_clienttime_and_serverdelay", m_nofSeries, &m_localTime, &diff);
        g_developmentMask &= ~DevelopmentMask::SaveMeasurementsSingle;
    }

    // sanitize the measurements. Remove the worst outliers
    // ----------------------------------------------------

    filtered_time.clear();
    filtered_diff.clear();
    int64_t average_offset_ns = 0.0;
    OffsetMeasurement::ResultCode resultCode = OffsetMeasurement::PASS;
    size_t filtered_samples_size = 0;

    switch (m_filterType)
    {
    case DEFAULT:
    {
        trace->critical("can't apply default filter..");
        resultCode = OffsetMeasurement::INTERNALERROR;
        break;
    }
    case EVERYTHING:
    {
        filtered_samples_size = diff.size();
        average_offset_ns = MathFunc::average(diff);
        break;
    }
    case LOWEST_VALUES:
    {
        const int range = 600000;
        int64_t minimum = MathFunc::min(diff);
        int64_t maximum = minimum + range;
        resultCode = filterMeasurementsInRange(
                    m_localTime, diff,
                    filtered_time, filtered_diff,
                    minimum, maximum,
                    average_offset_ns);
        if (resultCode == OffsetMeasurement::PASS)
        {
            resultCode = accept(diff.size(), filtered_diff.size(), 50.0, 10.0, 80.0, 50.0);
        }

        filtered_samples_size = filtered_time.size();
        break;
    }
    case LARGEST_BIN_WINDOW:
    {
        auto av = averageFromLargestHistogramBin(diff, 100, 1000000);
        int64_t minimum = av - 300000;
        int64_t maximum = av + 110000;

        SampleList64 new_diff;
        SampleList64 time;

        resultCode = filterMeasurementsInRange(
                         m_localTime, diff,
                         filtered_time, filtered_diff,
                         minimum, maximum,
                         average_offset_ns);

        if (resultCode == OffsetMeasurement::PASS)
        {
            resultCode = accept(diff.size(), filtered_diff.size(), 50.0, 10.0, 70.0, 50.0);
        }

        filtered_samples_size = filtered_time.size();
        break;
    }
    }

    if (resultCode != OffsetMeasurement::PASS && g_developmentMask & DevelopmentMask::SaveOnBailingOut)
    {
        DataFiles::dumpVectors("onbailingout_raw", m_nofSeries, &m_localTime, &diff);
        DataFiles::dumpVectors("onbailingout_filtered", m_nofSeries, &filtered_time, &filtered_diff);
    }

    OffsetMeasurement offsetMeasurement(m_nofSeries,
                                        filtered_time.front(), filtered_time.back(),
                                        m_measurementRun,
                                        m_localTime.size(),
                                        filtered_samples_size,
                                        average_offset_ns,
                                        resultCode);

    return offsetMeasurement;
}


// development
void BasicMeasurementSeries::setFiltering(BasicMeasurementSeries::FilterType filterType)
{
    m_filterType = filterType;
}


void BasicMeasurementSeries::saveRawMeasurements(std::string filename, int serial) const
{
    SampleList64 diff = MathFunc::diff(m_localTime, m_remoteTime);
    DataFiles::dumpVectors(filename.c_str(), serial, &m_localTime, &m_remoteTime, &diff);
}


void BasicMeasurementSeries::saveFilteredMeasurements(std::string filename, int serial) const
{
    DataFiles::dumpVectors(filename.c_str(), serial, &filtered_time, &filtered_diff);
}

