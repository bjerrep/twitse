#include "apputils.h"
#include "globals.h"
#include "log.h"

#include <QObject>


void timerOff(QObject* host, int& timerId)
{
    if (timerId != TIMEROFF)
    {
        host->killTimer(timerId);
        timerId = TIMEROFF;
    }
}


void timerOn(QObject* host, int& timerId, int period)
{
    timerOff(host, timerId);
    timerId = host->startTimer(period);
}
