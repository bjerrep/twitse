#pragma once

#include "multicast.h"

#include <QCoreApplication>
#include <QtNetwork/QUdpSocket>
#include <QUuid>
#include <QCommandLineParser>

class Control : public QObject
{
    Q_OBJECT

public:
    Control(QCoreApplication *parent, QCommandLineParser &parser, const QHostAddress &address, uint16_t port);

private:
    QCoreApplication* m_parent;
    Multicast* m_multicast;
    QString m_uid = QUuid::createUuid().toString();
};
