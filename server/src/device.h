#pragma once

#include "globals.h"
#include "systemtime.h"
#include "lock.h"
#include "log.h"
#include "rxpacket.h"
#include "lock.h"
#include "mathfunc.h"

#include <QString>
#include <QIODevice>
#include <QDataStream>
#include <QByteArray>
#include <QHostAddress>
#include <QJsonObject>
#include <QMutex>

class OffsetMeasurementHistory;
class MeasurementSeriesBase;
class QTcpServer;
class QTcpSocket;
class QUdpSocket;


class StatusReport
{
public:
    StatusReport()
    {
        m_startTime = s_systemTime->getRunningTime_secs();
    }

    void newMeasurement(int sent, int received, int used)
    {
        m_sent += sent;
        m_received += received;
        m_used += used;
    }

    void packetSentOrReceived()
    {
        m_nofPackets++;
    }

    std::string getReport() const
    {
        double elapsed = s_systemTime->getRunningTime_secs() - m_startTime;
        std::string loss = "loss ?";
        std::string used = "used ?";

        if (m_received)
        {
            double usedpct = 100.0 - 100.0 * (m_received - m_used) / m_received;
            used = fmt::format("used% {:.1f}", usedpct);
            double losspct = 100.0 * (m_sent - m_received) / m_sent;
            loss = fmt::format("loss% {:.1f}", losspct);
        }

        std::string ret = fmt::format("packet/sec {:.0f} {} {}", m_nofPackets/elapsed, loss, used);
        return ret;
    }

private:
    double m_startTime;
    int m_sent = 0;
    int m_received = 0;
    int m_used = 0;
    int m_nofPackets = 0;
};


enum InitState
{
    PPM_MEASUREMENTS,
    CLIENT_CONFIGURING,
    RUNNING
};

class Device : public QObject
{
    Q_OBJECT

public:
    Device(QObject *parent, const QString& name);
    ~Device();

    void tcpTx(const QJsonObject& json);
    void tcpTx(const QString& command);
    void sendServerTimeUDP();
    void processTimeSample(int64_t remoteTime, int64_t localTime);
    void processMeasurement(const RxPacket &rx);

    std::string name() const;
    void measurementStartRequest();
    void measurementStart();
    void getClientOffset();
    std::string getStatusReport();
    QJsonObject getLockAndQuality() const;
    void transmitLockAndQuality(bool force = false);

private:
    void clientDisconnected();
    void timerEvent(QTimerEvent *event);
    void measurementFinalize();
    std::string getLogName() const;

signals:
    void signalRequestSamples(Device*, int, int);
    void signalConnectionLost(QString name);
    void signalNewOffsetMeasurement(const QString&, double, double, double);
    void signalWebsocketTransmit(const QJsonObject& json);
    void signalRequestMeasurementStart(Device *device);
    void signalMeasurementFinalized(Device *device);

public slots:
    void slotSendStatus();

private slots:
    void slotTcpRx();
    void slotClientConnected();
    void slotUdpRx();
    void slotUpdatedLockAndQuality();

public:
    QString m_name;
    std::string m_logName;
    QTcpServer* m_server;
    QByteArray m_tcpReadBuffer;
    QTcpSocket *m_tcpSocket;
    int m_clientActiveTimer = TIMEROFF;
    int m_measurementSilencePeriodTimer = TIMEROFF;
    int m_clientPingTimer = TIMEROFF;
    int m_clientPingCounter = 0;
    QString m_serverAddress;
    QUdpSocket* m_udp;
    QHostAddress m_clientAddress;
    quint16 m_clientTcpPort;
    quint16 m_clientUdpPort;
    int m_udpOverruns = 0;
    bool m_clientConnected = false;
    bool m_clientActive = false;
    bool m_measurementStarted = false;

    int m_initStateCounter = NOF_INITIAL_PPM_MEASUREMENTS;
    InitState m_initState = InitState::PPM_MEASUREMENTS;

    MeasurementSeriesBase* m_measurementSeries;
    OffsetMeasurementHistory* m_offsetMeasurementHistory;

    double m_avgRoundtrip_us = 0.0;
    bool m_averagesInitialized = false;
    double m_avgClientOffset_ns = 0.0;
    double m_previousClientOffset_ns = 0.0;
    Lock m_lock;
    QJsonObject m_lastLockAndQualityMessage;
    double m_packageLossPct = 0.0;
    StatusReport m_statusReport;
    QMutex m_mutex;

    int m_fixedSamplePeriod_ms = -1;
    int m_saveOddMeasurementsThreshold_ns = 20000;
    bool m_saveOddMeasurements = true;
};
