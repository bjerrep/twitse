#include "mathfunc.h"
#include "log.h"

#include <iterator>
#include <numeric>

const int64_t NS_IN_MSEC = 1000000LL;


MathSample::MathSample(int64_t local, int64_t remote)
{
    m_localTime = local;
    m_remoteTime = remote;
    m_diff = local - remote;
}

// ---------------------------------------------------


void MathMeasurement::add(int64_t local, int64_t remote)
{
    m_measurement.push_back(MathSample(local, remote));
}


void MathMeasurement::add(const MathMeasurement& other)
{
    m_measurement.insert(std::end(m_measurement), std::begin(other.m_measurement), std::end(other.m_measurement));
}


void MathMeasurement::copy(const SampleList64& local, const SampleList64& remote)
{
    for (size_t i = 0; i < local.size(); i++)
    {
        m_measurement.push_back(MathSample(local[i], remote[i]));
    }
}


int64_t MathMeasurement::averageOffset_ns()
{
    double sum = 0;
    for (auto meas : m_measurement)
    {
        sum += meas.m_diff;
    }

    return sum / m_measurement.size();
}


size_t MathMeasurement::size() const
{
    return m_measurement.size();
}


MathSample MathMeasurement::minimum() const
{
    size_t smallest = 0;

    for (size_t i = 0; i < m_measurement.size(); i++)
    {
        if (m_measurement.at(i).m_diff < m_measurement.at(smallest).m_diff)
        {
            smallest = i;
        }
    }
    return m_measurement.at(smallest);
}


MeasurementVector& MathMeasurement::getMeasurementBin()
{
    return m_measurement;
}

// ---------------------------------------------------


MathMeasurementHistogram::MathMeasurementHistogram(size_t size, MathMeasurement measurement)
    : m_total(measurement.size())
{
    m_bins.resize(size);
    int64_t lower = measurement.minimum().m_diff;
    int64_t upper = lower + 5 * NS_IN_MSEC;
    int64_t width = (upper - lower) / (int64_t) size;

    for (auto meas : measurement.getMeasurementBin())
    {
        long index = (meas.m_diff - lower) / width;
        if (index >= 0 and (size_t) index < size)
        {
            m_bins.at(index).getMeasurementBin().push_back(meas);
        }
    }
    /*
    for (size_t i = 0; i < m_bins.size(); i++)
    {
        trace->info("{} {}", i, m_bins.at(i).size());
    }
    */
}


size_t MathMeasurementHistogram::getLargestBinIndex() const
{
    size_t index = 0;
    size_t max_len = 0;
    for (size_t i = 0; i < m_bins.size(); i++)
    {
        if (m_bins[i].size() >= max_len)
        {
            max_len = m_bins[i].size();
            index = i;
        }
    }
    return index;
}


bool MathMeasurementHistogram::getCentralPercentage(double percent, MathMeasurement& result)
{
    size_t index = getLargestBinIndex();
    result = m_bins[index];

    if (result.size() < 10)
    {
        trace->critical("quality too low in getCentralPercentage, bailing out");
        return false;
    }

    size_t up = index + 1;
    int down = index - 1;
    size_t requested_samples = m_total * percent / 100.0;

    while(result.size() < requested_samples && up < m_bins.size() && down >= 0)
    {
        result.add(m_bins[up++]);
        result.add(m_bins[(size_t) down--]);
    }
    return true;
}


// ---------------------------------------------------

bool MathFunc::linearRegression(const SampleList64 &_x, const SampleList64 &_y, double &slope, double &constant)
{
    std::vector<double> x;
    for( const int64_t& val : _x)
        x.push_back(val - _x.front());

    std::vector<double> y;
    for( const int64_t& val : _y)
        y.push_back(val - _y.front());

    double n = x.size();

    double avgX = accumulate(x.begin(), x.end(), 0.0) / n;
    double avgY = accumulate(y.begin(), y.end(), 0.0) / n;

    double numerator = 0.0;
    double denominator = 0.0;

    for(int i=0; i<n; ++i)
    {
        numerator += (x[i] - avgX) * (y[i] - avgY);
        denominator += (x[i] - avgX) * (x[i] - avgX);
    }
    slope = numerator / denominator;
    constant = - x[0] * slope;

    return true;
}

/// Uses zero mean so this is also/actually root mean square as well
///
double MathFunc::standardDeviation(const SampleList64 &samples)
{
    double avg = MathFunc::average(samples);
    size_t n = samples.size();
    double variance = 0.0;

    for(size_t i = 0; i < n; ++i)
    {
        variance += (avg - samples[i]) * (avg - samples[i]) / n;
    }
    return sqrt(variance);
}


double MathFunc::standardDeviation(const SampleList64 &_x, const SampleList64 &_y, double slope)
{
    std::vector<double> x;
    for( const int64_t& val : _x)
        x.push_back(val - _x.front());

    std::vector<double> y;
    for( const int64_t& val : _y)
        y.push_back(val - _y.front());

    size_t n = x.size();
    double variance = 0.0;

    for(size_t i = 0; i < n; ++i)
    {
        double mean = slope * x[i];
        variance += (mean - y[i]) * (mean - y[i]) / n;
    }
    return sqrt(variance);
}


double MathFunc::average(const SampleList64& samples)
{
    return accumulate(samples.begin(), samples.end(), 0.0) / samples.size();
}


int64_t MathFunc::min(const SampleList64& samples)
{
    return *std::min_element(std::begin(samples), std::end(samples));
}
