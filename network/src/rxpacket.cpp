#include "rxpacket.h"
#include "log.h"

#include <QJsonDocument>
#include <QDataStream>
#include <QJsonObject>
#include <ctime>


UdpRxPacket::UdpRxPacket(QByteArray& data, int64_t localTime)
    : m_localTime(localTime)
{
    m_remoteTime = *((int64_t*)(data.data()));
}


QString UdpRxPacket::toString() const
{
    return QString("Timestamp: recieved %1, local is %2").arg(m_remoteTime).arg(m_localTime);
}


RxPacket::RxPacket(QByteArray& deserializedjson)
{
    QJsonDocument doc(QJsonDocument::fromJson(deserializedjson));
    m_json = new QJsonObject(doc.object());
}


RxPacket::~RxPacket()
{
    delete m_json;
}


QString RxPacket::toString() const
{
    return QString("command '%1'").arg(value("command"));
}


int RxPacket::valueInt(const QString &key) const
{
    if (!m_json)
    {
        return -1;
    }
    auto value = m_json->value(key);
    return value.toInt();
}


QString RxPacket::value(const QString &key) const
{
    if (!m_json)
    {
        return QString();
    }
    auto value = m_json->value(key);
    return value.toString();
}


MulticastRxPacket::MulticastRxPacket(QByteArray& deserializedjson)
{
    QJsonDocument doc(QJsonDocument::fromJson(deserializedjson));
    m_json = new QJsonObject(doc.object());
}


QString MulticastRxPacket::toString() const
{
    return QString("command '%1'").arg(value("command"));
}


QString MulticastRxPacket::value(const QString &key) const
{
    if (!m_json)
    {
        return QString();
    }
    auto value = m_json->value(key);
    return value.toString();
}
