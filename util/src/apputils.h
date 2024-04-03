#pragma once
#include <string>

class QObject;

extern void timerOff(QObject* host, int& timerId);

extern void timerOn(QObject* host, int& timerId, int period);


namespace apputils
{
    std::string DeviceLabel(const std::string& devicename);

}