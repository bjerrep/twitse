#pragma once

#include <vector>
#include <stdint.h>

using SampleList64 = std::vector<int64_t>;

class MathFunc
{
public:
    static bool linearRegression(const SampleList64 &_x, const SampleList64 &_y, double &slope, double &constant);

    static double standardDeviation(const SampleList64 &samples);

    static double standardDeviation(const SampleList64 &_x, const SampleList64 &_y, double slope);

    static double average(const SampleList64& samples);
    static int64_t min(const SampleList64 &samples);
};
