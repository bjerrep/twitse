#include "mathfunc.h"
#include "log.h"

#include <numeric>


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

//    double aa = _x[0] * slope + constant;
//    double bb = _x.back() * slope + constant;
//    trace->info("first {}, last {}", _y[0] - aa, _y.back() - bb);

    return true;
}

double MathFunc::standardDeviation(const SampleList64 &samples)
{
    double avg = MathFunc::average(samples);
    int n = samples.size();
    double variance = 0.0;

    for(int i = 0; i < n; ++i)
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

    int n = x.size();
    double variance = 0.0;

    for(int i = 0; i < n; ++i)
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
