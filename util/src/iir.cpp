#include "iir.h"
#include <cmath>


void IIR::steadyState(double value)
{
    double prev = 0.0;
    for (int i = 0; i < 10000; i++)
    {
        double output = filter(value);
        if (i > 1000 && std::fabs(prev - output) < 0.0001 )
        {
            break;
        }
        prev = output;
    }
}


double IIR::filter(double value)
{
    xv[0] = xv[1]; xv[1] = xv[2]; xv[2] = xv[3]; xv[3] = xv[4];
    xv[4] = value / GAIN;
    yv[0] = yv[1]; yv[1] = yv[2]; yv[2] = yv[3]; yv[3] = yv[4];
    yv[4] = (xv[0] + xv[4]) + 4 * (xv[1] + xv[3]) + 6 * xv[2]
            + ( -0.1873794924 * yv[0]) + (  1.0546654059 * yv[1])
            + ( -2.3139884144 * yv[2]) + (  2.3695130072 * yv[3]);
    return yv[4];
}
