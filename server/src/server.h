#pragma once

#include "devicemanager.h"
#include "globals.h"
#include "i2c_access.h"

#include <QCoreApplication>
#include <QtNetwork/QUdpSocket>
#include <QUuid>

extern int g_developmentMask;

class Multicast;

class Server : public QObject
{
    Q_OBJECT

public:
    Server(QCoreApplication *parent, const QHostAddress &address, uint16_t port);

    void slotMulticastTx(MulticastTxPacket& tx);

    void executeControl(const MulticastRxPacket& rx);
    void processMetric(const MulticastRxPacket& rx);
    void printStatusReport();

public slots:
    void multicastRx(const MulticastRxPacket& rx);
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

    int m_wallAdjustTimer = TIMEROFF;
    int m_wallAdjustColdstartTimer = TIMEROFF;
    int m_wallSaveNewDefaultDAC = TIMEROFF;
    const int m_wallAdjustPeriodSecs = 30;
};
