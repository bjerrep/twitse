#include "client.h"
#include "log.h"
#include "globals.h"
#include "systemtime.h"
#include "interface.h"
#include "apputils.h"
#include "i2c_access.h"

#include <QObject>
#include <QThread>
#include <QTimer>
#include <QCommandLineParser>
#include <QNetworkDatagram>

extern DevelopmentMask g_developmentMask;
SystemTime* s_systemTime = nullptr;


Client::Client(QCoreApplication *parent, QString name,
               const QHostAddress &address, uint16_t port,
               spdlog::level::level_enum loglevel,
               bool no_clock_adj, bool autoPPM_LSB, double fixedPPM_LSB)
    : m_parent(parent),
      m_name(name),
      m_logLevel(loglevel),
      m_noClockAdj(no_clock_adj)
{
    s_systemTime = new SystemTime(false);

#ifdef VCTCXO
    m_i2c = new I2C_Access(1);
    m_i2c->writeVCTCXO_DAC(m_dacLSB);
    double luhab_temperature = m_i2c->readTemperature();
    trace->info("luhab carrier board temperature is {:.1f} °C", luhab_temperature);
#endif

    qRegisterMetaType<UdpRxPacketPtr>("UdpRxPacketPtr");
    qRegisterMetaType<TcpRxPacketPtr>("TcpRxPacketPtr");
    qRegisterMetaType<MulticastRxPacketPtr>("MulticastRxPacketPtr");

    reset();

    if (!autoPPM_LSB)
    {
        s_systemTime->setPPM(fixedPPM_LSB);
        m_dacLSB = fixedPPM_LSB;
        s_systemTime->setPPM(m_dacLSB);
        m_autoPPM_LSB_Adjust = false;
        trace->info("setting fixed ppm/lsb to to {}", fixedPPM_LSB);
    }

    m_multicastThread = new QThread(parent);
    m_multicast = new Multicast(name, address, port);

    connect(parent, &QCoreApplication::aboutToQuit, m_multicastThread, &QThread::quit);
    connect(m_multicastThread, &QThread::finished, m_multicastThread, &QThread::deleteLater);

    m_multicast->moveToThread(m_multicastThread);
    connect(m_multicast, &Multicast::rx, this, &Client::multicastRx);

    m_multicastThread->start();

    sendServerConnectRequest();

    m_SystemTimeRefreshTimer = startTimer(20);
}


Client::~Client()
{
    delete m_measurementSeries;
    delete m_i2c;
}


void Client::reset()
{
    trace->debug("client is resetting");
    m_connectionState = ConnectionState::NOT_CONNECTED;
    delete m_measurementSeries;
    m_measurementSeries = new BasicMeasurementSeries();
    m_offsetMeasurementHistory.reset();
    m_tcpSocket.close();
    if (m_udpSocket)
    {
        m_udpSocket->close();
        m_udpSocket->deleteLater();
        m_udpSocket = nullptr;
    }
    reconnectTimer(true);
    m_udpOverruns = 0;
    m_serverUid = "";
    m_setInitialLocalPPM = true;
    s_systemTime->reset();
    m_measurementInProgress = false;
#ifdef VCTCXO
    m_i2c->writeVCTCXO_DAC(m_dacLSB = 0x8000);
#endif
}


void Client::reconnectTimer(bool on)
{
    if (on)
    {
        if (m_tcpSocket.isOpen())
        {
            m_tcpSocket.close();
        }

        trace->info("contacting server");
        timerOn(this, m_reconnectTimer, g_clientReconnectPeriod);
        timerOff(this, m_serverPingTimer, true);
        timerOff(this, m_clientPingTimer, true);
    }
    else
    {
        if (m_reconnectTimer == TIMEROFF)
        {
            trace->error("reconnect timer already inactive?");
            return;
        }
        timerOff(this, m_reconnectTimer);
        timerOn(this, m_serverPingTimer, g_serverPingTimeout);
        timerOn(this, m_clientPingTimer, g_clientPingPeriod);
    }
}


void Client::multicastRx(MulticastRxPacketPtr rx)
{
    if (m_logLevel == spdlog::level::trace)
    {
        trace->trace("rx multicast: {}", rx->toString().toStdString());
    }

    if (m_connectionState == ConnectionState::CONNECTED)
    {
        if (rx->value("from") == "server" && rx->value("uid") != m_serverUid)
        {
            trace->warn("message from unknown server discarded");
            return;
        }
    }

    if (rx->value("from") == m_name || ((rx->value("to") != m_name && rx->value("to") != "all")))
    {
        return;
    }

    if (m_connectionState == ConnectionState::NOT_CONNECTED)
    {
        if (rx->value("command") == "serveraddress")
        {
            m_connectionState = ConnectionState::CONNECTING;
            m_serverUid = rx->value("uid");
            QHostAddress address(rx->value("tcpaddress"));
            uint16_t port = rx->value("tcpport").toInt();
            startTcpClient(address, port);
            reconnectTimer(false);
        }
    }
    else if (m_connectionState == ConnectionState::CONNECTED)
    {
        m_serverAlive = true;
    }

    if (rx->value("command") == "control")
    {
        executeControl(rx);
    }
}


void Client::executeControl(MulticastRxPacketPtr rx)
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
    else if (action == "vctcxodac")
    {
#ifdef VCTCXO
        QString value = rx->value("value");
        if (value == "auto")
        {
            trace->info("setting vctcxo to auto");
            m_autoPPM_LSB_Adjust = true;
        }
        else
        {
            uint16_t dac = value.toUShort();
            m_i2c->writeVCTCXO_DAC(dac);
            trace->info("setting vctcxo to fixed value {} (0x{:04x})", dac, dac);
            m_autoPPM_LSB_Adjust = false;
        }
#else
        trace->error("running in standalone mode, there is no dac here");
#endif
    }
    else
    {
        trace->warn("control command not recognized, {}", action);
    }
}


void Client::multicastTx(MulticastTxPacket tx)
{
    m_multicast->tx(tx);
}


void Client::udpRx()
{
    int64_t localTime = s_systemTime->getUpdatedSystemTime();
    int64_t remoteTime = *((int64_t*) m_udpSocket->receiveDatagram().data().data());

    m_measurementSeries->add(remoteTime, localTime);

    if (m_serverPingTimer != TIMEROFF)
    {
        m_serverAlive = true;
    }

    while (m_udpSocket->hasPendingDatagrams())
    {
        m_udpSocket->receiveDatagram();
        m_udpOverruns++;
    }

    usleep(1000);

    sendLocalTimeUDP();
}


void Client::tcpRx()
{
    m_serverAlive = true;

    m_tcpReadBuffer += m_tcpSocket.readAll();

    while(m_tcpReadBuffer.size() >= 2)
    {
        uint16_t length = *(uint16_t*) m_tcpReadBuffer.constData();
        if (m_tcpReadBuffer.size() < length + 2)
        {
            return;
        }

        QByteArray deserializedjson = m_tcpReadBuffer.mid(2, length);
        m_tcpReadBuffer.remove(0, length + 2);
        RxPacket rx(deserializedjson);

        QString command = rx.value("command");

        if (command == "sampleruncomplete")
        {
            transmitClientReady();
        }
        else if (command == "running")
        {
            m_measurementInProgress = true;
            m_expectedNofSamples = rx.value("samples").toInt();
            trace->debug("measurement started");
        }
        else if (command == "sendforwardoffset")
        {
            if (m_udpOverruns > 10)
            {
                trace->warn("discarded {} timesamples from udp overrun", m_udpOverruns);
            }
            m_udpOverruns = 0;

            QJsonObject json;
            json["command"] = "forwardoffset";
            OffsetMeasurement offsetMeasurement = finalizeMeasurementRun();
            json["offset"] = QString::number(offsetMeasurement.m_offset_ns);
            json["valid"] = offsetMeasurement.m_valid ? "true" : "false";
            trace->trace("send forwardoffset {} ns, valid {}",
                         offsetMeasurement.m_offset_ns,
                         offsetMeasurement.m_valid);
            tcpTx(json);

            m_measurementInProgress = false;
        }
        else if (command == "ping")
        {
        }
        else if (command == "adjustclock")
        {
            if (m_noClockAdj)
            {
                trace->info("noclockadj, not setting system time");
            }
            else
            {
                int64_t epoc_ns = rx.value("adjust_ns").toLongLong();

                if (epoc_ns)
                {
                    trace->info(WHITE "adjusting clock with {} ns" RESET, epoc_ns);

#ifdef CLIENT_USING_REALTIME
                    int64_t tt = s_systemTime->getWallClock() + epoc_ns;
                    struct timespec ts = {(__time_t) (tt / NS_IN_SEC), (__syscall_slong_t) (tt % NS_IN_SEC)};
                    clock_settime(CLOCK_REALTIME, &ts);
#else
                    s_systemTime->adjustSystemTime_ns(epoc_ns);
#endif
                }
                else
                {
                    trace->info(WHITE "not adjusting clock" RESET);
                }
                QJsonObject json;
                json["command"] = "clockadjusted";
                tcpTx(json);

                double local_ppm = m_offsetMeasurementHistory.getPPM();
                //double server_ppm = rx.value("set_ppm").toDouble();

                if (m_setInitialLocalPPM and m_autoPPM_LSB_Adjust)
                {
                    adjustPPM(local_ppm);
                    m_setInitialLocalPPM = false;
                }
            }
            if (VCTCXO_MODE)
            {
                int64_t wall_offset = rx.value("wall_clock").toLongLong();

                s_systemTime->setWallclock(s_systemTime->getRawSystemTime() + wall_offset);
                trace->info("adjusting wallclock to {}", s_systemTime->getWallClock());
            }
            m_offsetMeasurementHistory.reset();
            m_offsetMeasurementHistory.enableAverages();
        }
        else if (command == "adjustppm")
        {
            if (m_autoPPM_LSB_Adjust)
            {
                double ppm = rx.value("ppm_adjust").toDouble();
                adjustPPM(ppm);
            }
        }
        else if (command == "serverwallclock")
        {
            QJsonObject json; // fixit
            json["command"] = "clientwallclock";
            json["serverwallclock"] = rx.value("serverwallclock");
            json["clientwallclock"] = QString::number(s_systemTime->getWallClock());

            //trace->info("got server wallclock {}, returning client wallclock {}", rx.value("serverwallclock").toStdString(), s_systemTime->getWallClock()); // fixit, remove ?
            tcpTx(json);
        }
        else if (command == "adjustwallclock")
        {
            int64_t offset_ns = rx.value("offset_ns").toLongLong();
            s_systemTime->setWallclock(s_systemTime->getWallClock() - offset_ns);
        }
        else
        {
            trace->warn("noise on tcp socket '{}'", command.toStdString());
        }
    }
}


void Client::tcpTx(const QJsonObject &json)
{
    if (m_connectionState == ConnectionState::NOT_CONNECTED)
    {
        trace->warn("not connected, tcp tx ignored");
        return;
    }
    TcpTxPacket tx(json);
    m_tcpSocket.write(tx.getData());
}


void Client::tcpTx(const std::string& command)
{
    QJsonObject json;
    json["command"] = QString::fromStdString(command);
    tcpTx(json);
}


void Client::adjustPPM(double ppm)
{
#ifndef VCTCXO
    s_systemTime->setPPM(ppm);
#else
    if (!m_autoPPM_LSB_Adjust)
    {
        return;
    }
    // vctcxo "ASVTX-11-121-19.200MHz-T", nomimal +/-8ppm
    const double lsb_per_ppm = 2800.0;

    int64_t dacLSBcopy = m_dacLSB;

    m_dacLSB -= ppm * lsb_per_ppm;

    if (m_dacLSB < 0)
    {
        m_dacLSB = 0;
    }
    else if (m_dacLSB > 0xFFFF)
    {
        m_dacLSB = 0xFFFF;
    }
    if (m_dacLSB == 0 || m_dacLSB == 0xFFFF)
    {
        trace->critical("dac is saturated");
    }

    m_i2c->writeVCTCXO_DAC(m_dacLSB);
    trace->debug("ppm adjusted {} from {} to {} lsb", m_dacLSB - dacLSBcopy, dacLSBcopy, m_dacLSB);
#endif
}


void Client::connected()
{
    m_connectionState = ConnectionState::CONNECTED;
    connect(&m_tcpSocket, &QTcpSocket::readyRead, this, &Client::tcpRx);

    trace->info(IMPORTANT "connected, bind udp to local {}:{}" RESET,
                m_tcpSocket.localAddress().toString().toStdString(),
                m_tcpSocket.localPort());

    m_udpSocket = new QUdpSocket(this);
    m_udpSocket->bind(m_tcpSocket.localAddress(), m_tcpSocket.localPort());
    connect(m_udpSocket, SIGNAL(readyRead()), this, SLOT(udpRx()));

    tcpTx("ready");

    m_tcpSocket.setSocketOption(QAbstractSocket::LowDelayOption, 1);
}


void Client::timerEvent(QTimerEvent* timerEvent)
{
    int timerid = timerEvent->timerId();

    if (timerid == m_SystemTimeRefreshTimer)
    {
#ifndef VCTCXO
        if (!m_measurementInProgress)
        {
            s_systemTime->getUpdatedSystemTime();
        }
#endif
    }
    else if (timerid == m_reconnectTimer)
    {
        sendServerConnectRequest();
    }
    else if (timerid == m_serverPingTimer)
    {
        if (m_serverAlive)
        {
            m_serverAlive = false;
            return;
        }
        trace->error("**** server connection lost ****");
        reset();
    }
    else if (timerid == m_clientPingTimer)
    {
        tcpTx("ping");
    }
    else
    {
        trace->warn("got unexpected timer event");
    }
}


void Client::sendLocalTimeUDP()
{
    int64_t epoch = s_systemTime->getUpdatedSystemTime();
    m_udpSocket->writeDatagram((const char *) &epoch, sizeof(int64_t), m_serverAddress, m_serverTcpPort);
}


void Client::sendServerConnectRequest()
{
    trace->trace("sending connection request");
    QJsonObject json;
    json["from"] = m_name;
    json["to"] = "server";
    json["command"] = "connect";
    json["endpoint"] = Interface::getLocalAddress().toString();
    multicastTx(MulticastTxPacket(json));
}


void Client::startTcpClient(const QHostAddress &address, uint16_t port)
{
    trace->info("connecting, received server tcp socket address, connecting to {}:{}",
                address.toString().toStdString(), port);
    m_tcpSocket.connectToHost(address, port);
    connect(&m_tcpSocket, &QTcpSocket::connected, this, &Client::connected, Qt::UniqueConnection);
    m_serverAddress = address;
    m_serverTcpPort = port;
}


OffsetMeasurement Client::finalizeMeasurementRun()
{
    OffsetMeasurement summary = m_measurementSeries->calculate();
    if (summary.m_valid)
    {
        m_offsetMeasurementHistory.add(summary);
        trace->info(m_offsetMeasurementHistory.toString());
    }
    else
    {
        trace->warn(RED "measurement discarded" RESET);
    }

    return summary;
}


void Client::transmitClientReady()
{
    m_measurementSeries->prepareNewDataMeasurement(m_expectedNofSamples);
    tcpTx("ready");
}


/* Messages between server & client

client               server
                  <- running
                  <- (sending time)
(sending time) ->
                  ...
              ...

                  <- sendforwardoffset
forwardoffset ->

                  <- Optional: adjustclock
clockadjusted ->

                  <- sampleruncomplete
ready          ->
*/
