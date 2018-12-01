#pragma once

class QObject;

extern void timerOff(QObject* host, int& timerId);

extern void timerOn(QObject* host, int& timerId, int period);
