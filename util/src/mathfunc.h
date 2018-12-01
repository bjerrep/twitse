#pragma once

#include <vector>
#include <stdint.h>
#include <stddef.h>

using SampleList64 = std::vector<int64_t>;


class MathSample
{
public:
    MathSample(int64_t local, int64_t remote);

    int64_t m_localTime;
    int64_t m_remoteTime;
    int64_t m_diff;
};

using MeasurementVector = std::vector<MathSample>;


class MathMeasurement
{
public:
    void add(int64_t local, int64_t remote);
    void add(const MathMeasurement& other);
    void copy(const SampleList64& local, const SampleList64& remote);

    size_t size() const;

    int64_t averageOffset_ns();

    MathSample minimum() const;

    MeasurementVector& getMeasurementBin();

private:
    MeasurementVector m_measurement;
};


class MathMeasurementHistogram
{
public:
    MathMeasurementHistogram(size_t size, MathMeasurement measurement);

    size_t getLargestBinIndex() const;

    /// Returns as the minimum the samples from the largest bin. The percentage tells the minimum number of
    /// samples that should be present in the result. Until the percentage is satisfied
    /// bins in increasing distance (up and down) from the largest bin is added in pairs.
    bool getCentralPercentage(double percent, MathMeasurement& result);

    size_t m_total;
    std::vector<MathMeasurement> m_bins;
};


class MathFunc
{
public:
    static bool linearRegression(const SampleList64 &_x, const SampleList64 &_y, double &slope, double &constant);

    static double standardDeviation(const SampleList64 &samples);

    static double standardDeviation(const SampleList64 &_x, const SampleList64 &_y, double slope);

    static double average(const SampleList64& samples);

    static int64_t min(const SampleList64 &samples);
};
