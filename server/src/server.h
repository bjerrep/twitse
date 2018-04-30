#pragma once

#include "devicemanager.h"
#include "globals.h"

#include <QCoreApplication>
#include <QtNetwork/QUdpSocket>
#include <QUuid>

class Multicast;

class Server : public QObject
{
    Q_OBJECT

public:
    Server(QCoreApplication *parent, QString id, const QHostAddress &address, uint16_t port);

    ~Server();

    void slotMulticastTx(MulticastTxPacket& tx);

    void executeControl(MulticastRxPacketPtr rx);
    void printStatusReport();

public slots:
    void multicastRx(MulticastRxPacketPtr rx);
    void slotIdle();

private:
    void timerEvent(QTimerEvent *);

private:
    QCoreApplication* m_parent;
    QThread* m_multicastThread;
    Multicast* m_multicast;
    DeviceManager m_deviceManager;
    int m_statusReportTimer = TIMEROFF;
    bool m_pendingStatusReport = false;
    QString m_uid = QUuid::createUuid().toString();
};
