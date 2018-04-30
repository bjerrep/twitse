#include "server.h"
#include "log.h"
#include "globals.h"
#include "mathfunc.h"
#include "system.h"
#include "systemtime.h"
#include "device.h"

#include <QObject>
#include <QThread>
#include <QTimer>
#include <QCoreApplication>
#include <QCommandLineParser>
#include <QDateTime>

#include <iostream>

extern DevelopmentMask g_developmentMask;

int g_clientPeriod = -1;
int g_clientSamples = -1;

Server::Server(QCoreApplication *parent, QString id, const QHostAddress &address, uint16_t port)
    : m_parent(parent)
{
    m_multicastThread = new QThread(parent);
    m_multicast = new Multicast("no1", address, port);

    connect(parent, &QCoreApplication::aboutToQuit, m_multicastThread, &QThread::quit);
    connect(m_multicastThread, &QThread::finished, m_multicastThread, &QThread::deleteLater);
    qRegisterMetaType<UdpRxPacketPtr>("UdpRxPacketPtr");
    qRegisterMetaType<TcpRxPacketPtr>("TcpRxPacketPtr");
    qRegisterMetaType<MulticastRxPacketPtr>("MulticastRxPacketPtr");

    m_multicast->moveToThread(m_multicastThread);
    connect(m_multicast, &Multicast::rx, this, &Server::multicastRx);
    m_multicastThread->start();

    connect(&m_deviceManager, &DeviceManager::signalMulticastTx, this, &Server::slotMulticastTx );
    connect(&m_deviceManager, &DeviceManager::signalIdle, this, &Server::slotIdle );

    m_statusReportTimer = startTimer(g_statusReport);
    SystemTime::reset();
}


Server::~Server()
{
}


void Server::multicastRx(MulticastRxPacketPtr rx)
{
    if (rx->value("from") == "server"  || ((rx->value("to") != "server" && rx->value("to") != "all")))
    {
        return;
    }

    if (rx->value("command") == "control")
    {
        executeControl(rx);
    }
    else
    {
        m_deviceManager.process(rx);
    }
}

void Server::executeControl(MulticastRxPacketPtr rx)
{
    std::string action = rx->value("action").toStdString();
    trace->info("got control command {}", action);

    if (action == "kill")
    {
        trace->info("got kill, exiting..");
        QCoreApplication::quit();
    }
    else if (action == "developmentmask")
    {
        g_developmentMask = (DevelopmentMask) rx->value("developmentmask").toInt();
    }
    else if (action == "silence")
    {
        trace->info("setting period to {}", rx->value("value").toStdString());
        g_clientPeriod = rx->value("value") == "auto" ? -1 : rx->value("value").toInt();
    }
    else if (action == "samples")
    {
        trace->info("setting samples to {}", rx->value("value").toStdString());
        g_clientSamples = rx->value("value") == "auto" ? -1 : rx->value("value").toInt();
    }
    else
    {
        trace->warn("control command not recognized, {}", action);
    }
}


void Server::slotMulticastTx(MulticastTxPacket &tx)
{
    tx.setValue("from", "server");
    tx.setValue("uid", m_uid);

    m_multicast->tx(tx);
}


void Server::slotIdle()
{
    printStatusReport();
}


void Server::printStatusReport()
{
    if (m_pendingStatusReport && !m_deviceManager.activeClients())
    {
        m_pendingStatusReport = false;
        std::string dhms = QDateTime::fromTime_t(SystemTime::getRunningTime_secs()).toUTC().toString("dd:hh:mm:ss").toStdString();
        trace->info(CYAN "    runtime {}, cputemp {:.1f}" RESET,
                    dhms,
                    System::cpuTemperature());
        const DeviceDeque& devices = m_deviceManager.getDevices();
        trace->info(CYAN "    nof clients connected : {}" RESET, devices.size());
        for (Device* device : devices)
        {
            trace->info(CYAN "      {}" RESET, device->getStatusReport());
        }
    }
}


void Server::timerEvent(QTimerEvent* timerEvent)
{
    int id = timerEvent->timerId();

    if (id == m_statusReportTimer)
    {
        m_pendingStatusReport = true;
        printStatusReport();
    }
}
