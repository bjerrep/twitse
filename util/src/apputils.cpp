#include "apputils.h"
#include "globals.h"
#include "log.h"

#include <QObject>


void timerOff(QObject* host, int& timerId, bool quiet)
{
    if (timerId != TIMEROFF)
    {
        host->killTimer(timerId);
        timerId = TIMEROFF;
    }
    else if (!quiet)
    {
        trace->warn("timer already off");
    }
}


void timerOn(QObject* host, int& timerId, int period)
{
    if (timerId == TIMEROFF)
    {
        timerId = host->startTimer(period);
    }
    else
    {
        trace->warn("timer already on");
    }
}
