#pragma once

#include <vector>
#include <stdint.h>

using SampleList64 = std::vector<int64_t>;

class MathFifo
{
public:
    MathFifo(size_t length);
    int64_t getAverage() const;
    bool add(int64_t value);
    void reset();

private:
    SampleList64 m_fifo;
    size_t m_length;
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
