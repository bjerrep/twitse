#pragma once

#include "basicoffsetmeasurement.h"
#include "multicast.h"
#include "offsetmeasurementhistory.h"

#include "spdlog/common.h"
#include <QCoreApplication>
#include <QTcpSocket>
#include <QUdpSocket>

class I2C_Access;

class Client : public QObject
{
    Q_OBJECT

    enum ConnectionState
    {
        NOT_CONNECTED,
        CONNECTING,
        CONNECTED
    };

public:

    Client(QCoreApplication *parent, const QString &name,
           const QHostAddress &address, uint16_t port,
           spdlog::level::level_enum loglevel,
           bool clockadj, bool autoPPM_LSB, double fixedPPM_LSB);

    ~Client();

private:
    void reset(const QString& reason);
    OffsetMeasurement finalizeMeasurementRun();

    void sendServerConnectRequest();
    void transmitClientReady();
    void startTcpClient(const QHostAddress &address, uint16_t port);
    void processVCTCXOcommand(const MulticastRxPacket& rx);
    void multicastTx(MulticastTxPacket tx);
    void tcpTx(const QJsonObject& json);
    void tcpTx(const std::string& command);
    bool locked();
    void adjustPPM(double ppm);

    void reconnectTimer(bool on);
    void sendLocalTimeUDP();
    bool processIsTracking() const;
    bool processIsLocked() const;
    void executeControl(const MulticastRxPacket& rx);

    void timerEvent(QTimerEvent *);

private slots:
    void connected();
    void multicastRx(const MulticastRxPacket& rx);
    void tcpRx();
    void udpRx();

private:
    QCoreApplication* m_parent;
    QString m_id;
    spdlog::level::level_enum m_logLevel;
    bool m_autoPPMAdjust = true;
    bool m_noClockAdj;
    QString m_serverUid;

    int m_targetDAC = 0;

    QThread* m_multicastThread;
    Multicast* m_multicast = nullptr;
    QTcpSocket m_tcpSocket;
    QUdpSocket* m_udpSocket = nullptr;
    QHostAddress m_serverAddress;
    uint16_t m_serverTcpPort = 0;
    QByteArray m_tcpReadBuffer;
    int m_udpOverruns = 0;

    int m_reconnectTimer = TIMEROFF;
    int m_serverPingTimer = TIMEROFF;
    int m_clientPingTimer = TIMEROFF;
    int m_SystemTimeRefreshTimer = TIMEROFF;
    int m_saveNewDefaultDAC = TIMEROFF;
    int m_DACadjustmentTimer = TIMEROFF;

    BasicMeasurementSeries* m_measurementSeries = nullptr;
    OffsetMeasurementHistory m_offsetMeasurementHistory;

    ConnectionState m_connectionState = ConnectionState::NOT_CONNECTED;
    bool m_serverAlive = false;
    bool m_setInitialLocalPPM = true;
    bool m_measurementInProgress = false;
    int m_expectedNofSamples = 0;

    int m_lockCounter = 0;
    const int LOCK_MAX = 10;
};
