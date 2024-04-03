#include "client.h"
#include "defaultdac.h"
#include "log.h"
#include "globals.h"
#include "systemtime.h"
#include "interface.h"
#include "apputils.h"
#include "i2c_access.h"

#include <QObject>
#include <QThread>
#include <QTimer>
#include <QNetworkDatagram>
#include <cmath>

extern int g_developmentMask;
extern int g_randomTrashPromille;
SystemTime* s_systemTime = nullptr;


Client::Client(QCoreApplication *parent, const QString& id,
               const QHostAddress &address, uint16_t port,
               spdlog::level::level_enum loglevel,
               bool no_clock_adj, bool autoPPM_LSB, double fixedPPM_LSB)
    : m_parent(parent),
      m_id(id),
      m_logLevel(loglevel),
      m_noClockAdj(no_clock_adj)
{
    s_systemTime = new SystemTime(false);

    qRegisterMetaType<UdpRxPacketPtr>("UdpRxPacketPtr");
    qRegisterMetaType<TcpRxPacketPtr>("TcpRxPacketPtr");
    qRegisterMetaType<MulticastRxPacket>("MulticastRxPacket");

    reset("client starting");

    if (VCTCXO_MODE)
    {
        double luhab_temperature;
        auto success = I2C_Access::I2C()->readTemperature(luhab_temperature);

        if (success)
        {
            trace->info("luhab carrier board temperature is {:.1f} °C", luhab_temperature);
        }
        else
        {
            trace->info("could not read carrier board temperature (thats ok)");
        }

        if (autoPPM_LSB)
        {
            I2C_Access::I2C()->writeVCTCXO_DAC(loadDefaultDAC());
        }
        else
        {
            I2C_Access::I2C()->writeVCTCXO_DAC(fixedPPM_LSB);
            I2C_Access::I2C()->setFixedVCTCXO_DAC(true);
            trace->info("setting fixed dac to to {}", fixedPPM_LSB);
        }
        m_targetDAC = I2C_Access::I2C()->getVCTCXO_DAC();
    }
    else // software
    {
        if (!autoPPM_LSB)
        {
            s_systemTime->setPPM(fixedPPM_LSB);
            m_autoPPMAdjust = false;
            trace->info("setting fixed ppm/lsb to to {}", fixedPPM_LSB);
        }
    }

    m_multicastThread = new QThread(parent);
    m_multicast = new Multicast(id, address, port);

    connect(parent, &QCoreApplication::aboutToQuit, m_multicastThread, &QThread::quit);
    connect(m_multicastThread, &QThread::finished, m_multicastThread, &QThread::deleteLater);

    m_multicast->moveToThread(m_multicastThread);
    connect(m_multicast, &Multicast::rx, this, &Client::multicastRx);

    m_multicastThread->start();

    sendServerConnectRequest();

    m_SystemTimeRefreshTimer = startTimer(TIMER_20MS);
    // Outcomment to disable gradual DAC adjustments.
    m_DACadjustmentTimer = startTimer(TIMER_1SEC);
}


Client::~Client()
{
    delete m_measurementSeries;
    I2C_Access::I2C()->exit();
}


void Client::reset(const QString& reason)
{
    trace->debug("client is resetting: {}", reason.toStdString());
    m_connectionState = ConnectionState::NOT_CONNECTED;
    delete m_measurementSeries;

    m_measurementSeries = new BasicMeasurementSeries(m_id.toStdString());
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

    if (VCTCXO_MODE)
    {
        if (m_saveNewDefaultDAC != TIMEROFF)
        {
            killTimer(m_saveNewDefaultDAC);
        }
        m_saveNewDefaultDAC = startTimer(TIMER_5MIN);
        I2C_Access::I2C()->writeVCTCXO_DAC(loadDefaultDAC());
    }
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
        timerOff(this, m_serverPingTimer);
        timerOff(this, m_clientPingTimer);
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


void Client::multicastRx(const MulticastRxPacket& rx)
{
    if (m_connectionState == ConnectionState::CONNECTED)
    {
        if (rx.value("from") == "server" && rx.value("uid") != m_serverUid)
        {
            trace->warn("message from unknown server discarded");
            return;
        }
    }

    if (rx.value("from") == m_id || ((rx.value("to") != m_id && rx.value("to") != "all")))
    {
        trace->trace("rx multicast (discarded): {}", rx.toString().toStdString());
        return;
    }

    trace->debug("rx multicast: {}", rx.toString().toStdString());

    if (m_connectionState == ConnectionState::NOT_CONNECTED)
    {
        if (rx.value("command") == "serveraddress")
        {
            m_connectionState = ConnectionState::CONNECTING;
            m_serverUid = rx.value("uid");
            QHostAddress address(rx.value("tcpaddress"));
            uint16_t port = rx.value("tcpport").toUShort();
            startTcpClient(address, port);
            reconnectTimer(false);
        }
    }
    else if (m_connectionState == ConnectionState::CONNECTED)
    {
        m_serverAlive = true;
    }

    if (rx.value("command") == "control")
    {
        executeControl(rx);
    }
}


void Client::executeControl(const MulticastRxPacket& rx)
{
    std::string action = rx.value("action").toStdString();
    trace->trace("got control command {}", action);

    if (action == "kill")
    {
        trace->info("got kill, exiting..");
        QCoreApplication::quit();
    }
    else if (action == "developmentmask")
    {
        g_developmentMask = rx.value("developmentmask").toInt();
    }
    else if (action == "vctcxodac")
    {
#ifdef VCTCXO
        processVCTCXOcommand(rx);
#else
        trace->error("running in software mode, there is no dac here");
#endif
    }
    else
    {
        trace->warn("control command not recognized, {}", action);
    }
}


void Client::processVCTCXOcommand(const MulticastRxPacket& rx)
{
    QString value = rx.value("value");
    if (value == "auto")
    {
        trace->info("setting vctcxo to auto");
        I2C_Access::I2C()->setFixedVCTCXO_DAC(false);
    }
    else if (value == "get")
    {
        QJsonObject json;
        json["from"] = m_id;
        json["to"] = rx.value("from");
        json["command"] = "metric";
        json["action"] = "vctcxo_dac";
        json["value"] = QString::number(I2C_Access::I2C()->getVCTCXO_DAC());
        multicastTx(MulticastTxPacket(json));

        trace->trace("sending vctcxo dac, {}={}", m_id.toStdString(), I2C_Access::I2C()->getVCTCXO_DAC());
    }
    else
    {
        uint16_t dac = value.toUShort();
        I2C_Access::I2C()->writeVCTCXO_DAC(dac);
        I2C_Access::I2C()->setFixedVCTCXO_DAC(true);
        trace->info("setting vctcxo to fixed value {} (0x{:04x})", dac, dac);
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

    sendLocalTimeUDP();
}


void Client::tcpRx()
{
    m_serverAlive = true;

    m_tcpReadBuffer += m_tcpSocket.readAll();

    while(m_tcpReadBuffer.size() >= 2)
    {
        uint16_t length = *(const uint16_t*) m_tcpReadBuffer.constData();
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
            json["valid"] = QString(OffsetMeasurement::ResultCodeAsString(offsetMeasurement.resultCode()).c_str());

            trace->trace("send forwardoffset {} ns, result {}",
                         offsetMeasurement.m_offset_ns,
                         OffsetMeasurement::ResultCodeAsString(offsetMeasurement.resultCode()));
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

                if (m_setInitialLocalPPM and m_autoPPMAdjust)
                {
                    adjustPPM(local_ppm);
                    m_setInitialLocalPPM = false;
                }
            }
            if (VCTCXO_MODE)
            {
                int64_t wall_offset = rx.value("wall_clock").toLongLong();

                s_systemTime->setWallclock_ns(s_systemTime->getRawSystemTime_ns() + wall_offset);
                trace->info("adjusting wallclock to {}", SystemTime::getWallClock_ns());
            }
            m_offsetMeasurementHistory.reset();
        }
        else if (command == "adjustppm")
        {
            if (m_autoPPMAdjust)
            {
                double ppm = rx.value("ppm_adjust").toDouble();
                adjustPPM(ppm);
            }
        }
        else if (command == "adjustwallclock")
        {
            int64_t offset_ns = rx.value("offset_ns").toLongLong();
            s_systemTime->setWallclock_ns(SystemTime::getWallClock_ns() - offset_ns);
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


bool Client::locked()
{
    return m_lockCounter == LOCK_MAX;
}


/* During normal operation the client autonomously adjusts its vctcxo dac
 * based on a relative ppm value from the server. The idea is that it is the client
 * that knows e.g. the sensitivity of its own vctcxo oscillator when converting a ppm
 * value to a dac adjustment.
 */
void Client::adjustPPM(double ppm)
{
    if (VCTCXO_MODE)
    {
        if (!m_autoPPMAdjust)
        {
            return;
        }

        std::string extras;

        // Local detection of a stable time lock. A stable time lock is the criteria for
        // saving a new updated default dac to file. Note that the server currently have
        // a limit of relative ppm adjustments of +/- 0.1 as a protection against spurious
        // bogus measurements.
        if (std::fabs(ppm) < 0.08)
        {
            if( m_lockCounter != LOCK_MAX )
            {
                if (++m_lockCounter == LOCK_MAX)
                {
                    extras = ". Hard lock entered";
                }
            }
        }
        else
        {
            if (m_lockCounter == LOCK_MAX)
            {
                extras = ". Hard lock lost";
            }
            m_lockCounter = 0;
        }

        // vctcxo "ASVTX-11-121-19.200MHz-T", nomimal +/-8ppm
        // Rather than a constant the lsb_per_ppm should be a calculation based on real measurements
        // on the given vctcxo. It would be nice if it at least covered the expected nonlinearities
        // at the extremes.
        const double lsb_per_ppm = 2600.0;

        int64_t dac = I2C_Access::I2C()->getVCTCXO_DAC();

        int new_dac = dac - ppm * lsb_per_ppm;

        new_dac = qBound(0, new_dac, 0xFFFF);

        if (new_dac == 0 || new_dac == 0xFFFF)
        {
            trace->critical("dac is saturated");
        }

        if (m_DACadjustmentTimer != TIMEROFF)
        {
            // let the timer make gradual adjustments. Sounds like a good idea
            // unless e.g. the DAC output itself has a noise component from each i2c write...
            // The jury is still out regarding this one at time of writing.
            m_targetDAC = new_dac;
        }
        else
        {
            // just write the new dac in one go. This is the brutal one.
            I2C_Access::I2C()->writeVCTCXO_DAC(new_dac);
        }

        trace->info("adjust {:.6f} ppm as DAC {} lsb (from {} to {}){}", ppm, new_dac - dac, dac, new_dac, extras);
    }
    else // std
    {
        s_systemTime->setPPM(ppm);
    }
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
    else if (timerid == m_DACadjustmentTimer)
    {
        if (!I2C_Access::I2C()->getFixedVCTCXO_DAC())
        {
            int current_dac = I2C_Access::I2C()->getVCTCXO_DAC();
            int diff = m_targetDAC - current_dac;
            if (diff)
            {
                int diff_rate = diff / 5;
                if (!diff_rate)
                {
                    diff_rate = diff > 0 ? 1 : -1;
                }
                I2C_Access::I2C()->adjustVCTCXO_DAC(diff_rate);
                trace->debug("adjust dac gradual with {}", diff_rate);
            }
        }
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
        reset("server lost");
    }
    else if (timerid == m_clientPingTimer)
    {
        tcpTx("ping");
    }
    else if (VCTCXO_MODE && timerid == m_saveNewDefaultDAC)
    {
        if (locked())
        {
            uint16_t dac = ((int) loadDefaultDAC() + I2C_Access::I2C()->getVCTCXO_DAC()) / 2;
            saveDefaultDAC(dac);
            killTimer(m_saveNewDefaultDAC);
            m_saveNewDefaultDAC = TIMEROFF;
        }
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
    json["from"] = m_id;
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
    if (summary.resultCode() == OffsetMeasurement::PASS)
    {
        m_offsetMeasurementHistory.add(summary);
        trace->info(m_offsetMeasurementHistory.clientToString(I2C_Access::I2C()->getVCTCXO_DAC()));
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
