#pragma once

#include <QObject>
#include <QSharedPointer>
#include <QString>

class QJsonObject;


class RxPacket
{
public:
    RxPacket() {}
    RxPacket(QByteArray &deserializedjson);
    virtual ~RxPacket();

    virtual QString toString() const;
    int valueInt(const QString &key) const;
    QString value(const QString& key) const;

private:
    QJsonObject* m_json = nullptr;
};


class MulticastRxPacket
{
public:
    MulticastRxPacket() {}
    MulticastRxPacket(QByteArray &deserializedjson);
    virtual ~MulticastRxPacket() = default;

    virtual QString toString() const;
    QString value(const QString& key) const;

private:
    QJsonObject* m_json = nullptr;
};


class UdpRxPacket
{
public:
    UdpRxPacket() {}
    UdpRxPacket(QByteArray& data, int64_t rx_time);

    QString toString() const;

private:
    int64_t m_localTime;
    int64_t m_remoteTime;
};

using UdpRxPacketPtr = QSharedPointer<UdpRxPacket>;
using TcpRxPacketPtr = QSharedPointer<RxPacket>;

Q_DECLARE_METATYPE(UdpRxPacketPtr)
Q_DECLARE_METATYPE(TcpRxPacketPtr)
