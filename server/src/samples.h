#pragma once

#include <QObject>
#include <QList>
#include <QJsonObject>
#include <QTimerEvent>

class ClientSampleRun
{
public:
    ClientSampleRun(const QString& client, int count);
    ~ClientSampleRun();

    QString m_name;
    int m_count = 0;
    double m_startTime;
};


class Samples : public QObject
{
    Q_OBJECT

    const int TIMEROFF = -1;

public:
    Samples();

    void setPeriod(int period_ms);
    void timerEvent(QTimerEvent*);
    void removeClient(const QString &clientname);

public slots:
    void slotRequestSamples(const QString &clientname, int count, int period_ms);

signals:
    void signalSendTimeSample(const QString& clientname);
    void signalSampleRunStatusUpdate(const QString& clientname, bool active);

private:
    QList<ClientSampleRun*> m_sampleRuns;
    QList<ClientSampleRun*>::Iterator m_client_it = nullptr;
    int m_timerId = TIMEROFF;
    int m_period_ms = 0;
};
