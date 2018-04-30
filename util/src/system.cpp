#include "system.h"
#include "log.h"
#include <iostream>
#include <fstream>


double System::cpuTemperature()
{
    std::ifstream istream("/sys/devices/virtual/thermal/thermal_zone0/temp");
    char output[100];
    istream.getline(output, 100);
    double temp = atoi(output) / 1000.0;
    return temp;
}
