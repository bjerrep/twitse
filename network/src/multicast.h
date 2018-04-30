#pragma once

#include "rxpacket.h"
#include "txpacket.h"

#include <time.h>
#include <QObject>
#include <QtNetwork/QUdpSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSharedPointer>


class Multicast : public QObject
{
    Q_OBJECT

public:
    Multicast(QString id, const QHostAddress &address, uint16_t port);

    void tx(MulticastTxPacket &tx);

signals:
    void rx(MulticastRxPacketPtr rx);

public slots:
    void slotMulticastRx();

private:
    QString m_id;
    QHostAddress m_hostAddress;
    uint16_t m_port;
    QUdpSocket* m_multicastSocket;
};
