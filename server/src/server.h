#pragma once

#include "devicemanager.h"
#include "globals.h"
#include "iir.h"

#include <QCoreApplication>
#include <QtNetwork/QUdpSocket>
#include <QUuid>

extern DevelopmentMask g_developmentMask;

class Multicast;

class Server : public QObject
{
    Q_OBJECT

    enum SoftPPMState {
        INITIALIZE_PPM,
        FIRST_ADJUSTMENT,
        RUNNING
    };

public:
    Server(QCoreApplication *parent, const QHostAddress &address, uint16_t port);

    ~Server();

    void slotMulticastTx(MulticastTxPacket& tx);

    void executeControl(MulticastRxPacketPtr rx);
    void printStatusReport();

public slots:
    void multicastRx(MulticastRxPacketPtr rx);
    void slotIdle();

private:
    void startServer();
    void adjustRealtimeSoftPPM();
    void timerEvent(QTimerEvent *);

private:
    QCoreApplication* m_parent;
    QHostAddress m_address;
    uint16_t m_port;
    QThread* m_multicastThread;
    Multicast* m_multicast;
    DeviceManager m_deviceManager;
    int m_statusReportTimer = TIMEROFF;

    bool m_pendingStatusReport = false;
    QString m_uid = QUuid::createUuid().toString();

    int m_softPPMAdjustTimer = TIMEROFF;
    int m_softPPMColdstartTimer = TIMEROFF;
    int m_softPPMAdjustPeriodSecs = 10;
    double m_softPPMPreviousDiff = 0.0;
    double m_softPPMAdjustLimit = 0.1;
    bool m_softPPMAdjustLimitActive = true;
    SoftPPMState m_softPPMInitState = INITIALIZE_PPM;
    int m_softPPMLoopCounter = 20;
    int64_t m_softPPMPrevAdjustTime;
    IIR m_iir;
};
