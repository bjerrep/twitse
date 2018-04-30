#pragma once

#include "globals.h"
#include "systemtime.h"
#include "lock.h"
#include "log.h"
#include "rxpacket.h"

#include <QString>
#include <QIODevice>
#include <QDataStream>
#include <QByteArray>
#include <QHostAddress>

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
        m_startTime = SystemTime::getRunningTime_secs();
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
        double elapsed = SystemTime::getRunningTime_secs() - m_startTime;
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


class Device : public QObject
{
    Q_OBJECT

public:
    Device(QObject *parent, const QString& name, const QHostAddress & address = QHostAddress::Any);
    ~Device();

    void tcpTx(const QJsonObject& json);
    void tcpTx(const QString& command);
    void sendServerTimeUDP();
    void processTimeSample(int64_t serverTime, int64_t localTime);
    void processMeasurement(const RxPacket &rx);

    std::string name() const;
    void prepareSampleRun();
    void getClientOffset();
    std::string getStatusReport();

private:
    void clientDisconnected();
    void timerEvent(QTimerEvent *event);
    void sampleRunComplete();

signals:
    void signalRequestSamples(QString name, int, int);
    void signalConnectionLost(QString name);

private slots:
    void slotTcpRx();
    void slotClientConnected();
    void slotUdpRx();

public:
    QString m_name;
    QTcpServer* m_server;
    QByteArray m_tcpReadBuffer;
    QTcpSocket *m_socket;
    int m_clientActiveTimer = TIMEROFF;
    int m_sampleRunTimer = TIMEROFF;
    int m_clientPingTimer = TIMEROFF;
    int m_clientPingCounter = 0;
    QString m_serverAddress;
    QUdpSocket* m_udp;
    QHostAddress m_clientAddress;
    quint16 m_clientTcpPort;
    quint16 m_clientUdpPort;

    MeasurementSeriesBase* m_measurementSeries;
    OffsetMeasurementHistory* m_offsetMeasurementHistory;
    double m_avgRoundtrip_us = 0.0;
    int m_udpOverruns = 0;
    int m_clockAdjust = 3;
    bool m_averagesInitialized = false;
    double m_avgClientOffset = 0.0;
    bool m_ppmActive = false;
    double m_prev = 0.0;
    Lock m_lock;
    StatusReport m_statusReport;
    bool m_clientConnected = false;
    bool m_clientActive = false;
};
