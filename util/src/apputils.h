#pragma once

class QObject;

extern void timerOff(QObject* host, int& timerId, bool quiet = false);

extern void timerOn(QObject* host, int& timerId, int period);
