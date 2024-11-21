#pragma once
#include <QString>
#include <QVector>
#include <QThread>
#include <QRecursiveMutex>


class Device;


class MeasurementSequencer : public QObject
{
    Q_OBJECT

public:
    MeasurementSequencer(QObject* parent);
    void reset();

private:
    int calcConcurrentMeasurements() const;
    void overloaded();
    void notOverloaded();

public slots:
    void slotRequestMeasurementStart(Device *device);
    void slotMeasurementFinalized(Device *device);

private:
    const int MAX_CONCURRENT_MEASUREMENTS = 3;
    const int FILTER_STEPS = 3;
    const int MAX_CONCURRENT_FILTER = MAX_CONCURRENT_MEASUREMENTS * FILTER_STEPS - 1;
    int m_maxConcurrentFilter = MAX_CONCURRENT_FILTER;
    QRecursiveMutex m_mutex;
    QVector<Device*> m_activeDevices;
    QVector<Device*> m_pendingDevices;
};
