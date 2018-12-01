#include "txpacket.h"
#include "log.h"

#include <QJsonDocument>
#include <QDataStream>
#include <QMap>
#include <QVector>
#include <ctime>


QString UdpTxPacket::toString() const
{
    return "none";
}

// ----------------------------------------------------------------------------


TcpTxPacket::TcpTxPacket(const QJsonObject &json)
    : m_json(json)
{
}


TcpTxPacket::TcpTxPacket(const QString &command)
{
    m_json["command"] = command;
}


TcpTxPacket::TcpTxPacket(const std::string &command)
{
    m_json["command"] = command.c_str();
}


QString TcpTxPacket::value(const QString &key) const
{
    if (m_json.isEmpty())
        return QString();
    return m_json.value(key).toString();
}


void TcpTxPacket::setValue(const QString &key, const QString &value)
{
    m_json[key] = value;
}


QByteArray TcpTxPacket::getData(bool serialize)
{
    QJsonDocument doc(m_json);
    QByteArray tx = doc.toJson();

    uint16_t length = tx.size();

    QByteArray header((char*) &length, sizeof(uint16_t));

    return header + tx;
}


QString TcpTxPacket::toString() const
{
    return QString("command %1").arg(value("command"));
}

// ----------------------------------------------------------------------------


MulticastTxPacket::MulticastTxPacket(const QJsonObject &json)
    : m_json(json)
{
}


MulticastTxPacket::MulticastTxPacket(const QString &command)
{
    m_json["command"] = command;
}


MulticastTxPacket::MulticastTxPacket(const QMap<QString, QString> &keyValues)
{
    for (const auto& kv : keyValues.toStdMap())
    {
        setValue(kv.first, kv.second);
    }
}

QString MulticastTxPacket::value(const QString &key) const
{
    if (m_json.isEmpty())
        return QString();
    return m_json.value(key).toString();
}


void MulticastTxPacket::setValue(const QString &key, const QString &value)
{
    m_json[key] = value;
}


QByteArray MulticastTxPacket::getData(bool serialize)
{
    QJsonDocument doc(m_json);
    return doc.toJson();
}


QString MulticastTxPacket::toString() const
{
    return QString("command %1").arg(value("command"));
}
