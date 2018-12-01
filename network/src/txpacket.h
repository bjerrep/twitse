#pragma once

#include <QJsonObject>
#include <QString>
#include <QByteArray>

using KeyVal = QMap<QString, QString>;

class TcpTxPacket
{
public:
    TcpTxPacket(const QJsonObject& json);
    TcpTxPacket(const QString &command);
    TcpTxPacket(const std::string &command);
    virtual ~TcpTxPacket() = default;

    virtual QByteArray getData(bool serialize = true);

    virtual QString toString() const;

    QString value(const QString &key) const;
    void setValue(const QString& key, const QString& value);

private:
    QJsonObject m_json;
};


class MulticastTxPacket
{
public:
    MulticastTxPacket() {}
    MulticastTxPacket(const QJsonObject& json);
    MulticastTxPacket(const QString &command);
    MulticastTxPacket(const KeyVal &keyValues);
    virtual ~MulticastTxPacket() = default;

    virtual QByteArray getData(bool serialize = true);

    virtual QString toString() const;

    QString value(const QString &key) const;
    void setValue(const QString& key, const QString& value);

private:
    QJsonObject m_json;
};


class UdpTxPacket
{
public:
    UdpTxPacket();

    QString toString() const;
};
