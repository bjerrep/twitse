#include "system.h"
#include "log.h"
#include <iostream>
#include <fstream>
#include <QProcess>


double System::cpuTemperature()
{
    std::ifstream istream("/sys/devices/virtual/thermal/thermal_zone0/temp");
    char output[100];
    istream.getline(output, 100);
    double temp = atoi(output) / 1000.0;
    return temp;
}

bool System::ntpSynced()
{
    QProcess ntp;
    ntp.start("sh -c \"/usr/bin/timedatectl show\"");
    ntp.waitForFinished();
    bool is_in_sync = ntp.readAll().indexOf("NTPSynchronized=yes") != -1;
    return is_in_sync;
}
