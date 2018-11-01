#pragma once

/* Digital filter designed by mkfilter/mkshape/gencode   A.J. Fisher
   Command line: /www/usr/fisher/helpers/mkfilter -Bu -Lp -o 4 -a 1.0000000000e-01 0.0000000000e+00 -l */

// https://www-users.cs.york.ac.uk/~fisher/cgi-bin/mkfscript

#define NZEROS 4
#define NPOLES 4
#define GAIN   2.072820954e+02

class IIR
{
public:
    void steadyState(double value);

    double filter(double value);

private:
    double xv[NZEROS+1] = {0.0};
    double yv[NPOLES+1] = {0.0};
};
