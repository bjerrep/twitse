#include "device.h"
#include "multicast.h"
#include "log.h"
#include "globals.h"
#include "systemtime.h"
#include "interface.h"
#include "basicoffsetmeasurement.h"
#include "offsetmeasurementhistory.h"
#include "apputils.h"
#include "datafiles.h"

#include <cmath>
#include <QObject>
#include <QTimerEvent>
#include <QNetworkDatagram>
#include <QTcpServer>
#include <QTcpSocket>


extern DevelopmentMask g_developmentMask;


Device::Device(QObject* parent, const QString& clientName, const QHostAddress& address)
    : QObject(parent),
      m_name(clientName),
      m_server(new QTcpServer(this)),
      m_udp(new QUdpSocket(this)),
      m_lock(clientName.toStdString())
{
    m_offsetMeasurementHistory = new OffsetMeasurementHistory;
    if (!m_server->listen())
    {
        trace->critical("{}unable to start device server", getLogName());
        return;
    }

    m_measurementSeries = new BasicMeasurementSeries(getLogName());
    m_serverAddress = Interface::getLocalAddress().toString();
    m_clientUdpPort = m_server->serverPort();
    trace->info("{}bind udp to local {}:{}", getLogName(), m_serverAddress.toStdString(), m_clientUdpPort);
    m_udp->bind(QHostAddress(m_serverAddress), m_clientUdpPort);
    connect(m_udp, SIGNAL(readyRead()), this, SLOT(slotUdpRx()));

    trace->info("{}started tcp server on {}:{}",
                getLogName(),
                m_serverAddress.toStdString(),
                m_server->serverPort());

    connect(m_server, &QTcpServer::newConnection, this, &Device::slotClientConnected);

    m_clientActiveTimer = startTimer(g_clientPingTimeout);
    m_clientPingTimer = startTimer(500);
    m_clientPingCounter = g_serverPingPeriod / 500;

    connect(&m_lock, &Lock::signalNewLockState, this, &Device::slotNewLockState);
}


Device::~Device()
{
    if (m_clientActiveTimer != TIMEROFF)
    {
        killTimer(m_clientActiveTimer);
    }

    m_server->close();
    m_server->deleteLater();
    m_udp->close();
    m_udp->deleteLater();

    delete m_offsetMeasurementHistory;
}


void Device::sendServerTimeUDP()
{
    int64_t epoch = s_systemTime->getUpdatedSystemTime();
    m_udp->writeDatagram((const char *) &epoch, sizeof(int64_t), m_clientAddress, m_clientTcpPort);
    m_clientPingCounter = g_serverPingPeriod / 500;
    m_statusReport.packetSentOrReceived();
}


void Device::processTimeSample(int64_t remoteTime, int64_t localTime)
{
    m_measurementSeries->add(remoteTime, localTime);
}


void Device::processMeasurement(const RxPacket& rx)
{
    OffsetMeasurement measurement = m_measurementSeries->calculate();

    m_statusReport.newMeasurement(m_lock.getNofSamples(),
                                  measurement.m_collectedSamples,
                                  measurement.m_usedSamples);

    trace->debug("{}{}", getLogName(), measurement.toString());

    int64_t server2client_ns = rx.value("offset").toLongLong();
    double client_us = server2client_ns / 1000.0;
    int64_t client2server_ns = measurement.m_offset_ns;
    double local_us = client2server_ns / 1000.0;

    int64_t roundtrip_ns = server2client_ns + client2server_ns;
    double roundtrip_us = roundtrip_ns / 1000.0;
    double roundtrip_ms = roundtrip_us / 1000.0;

    // the central offset from a measurement series
    int64_t clientoffset_ns = (client2server_ns - server2client_ns) / 2;

    measurement.setOffset_ns(clientoffset_ns);
    m_offsetMeasurementHistory->add(measurement);

    // initial guards for sanity. If any one fails then give up.

    bool valid = true;

    if (!(g_developmentMask & DevelopmentMask::SameHost) and (roundtrip_ms < 0.7 or roundtrip_ms > 5.0))
    {
        trace->critical("{}either system times are invalid or network is useless, roundtrip is {:.3f} msecs. Aborting", getLogName(), roundtrip_ms);
        valid = false;
    }

    if (valid and m_lock.isLock())
    {
        if (roundtrip_us / m_avgRoundtrip_us > 2.0)
        {
            trace->critical("{}roundtrip jumped from average {:.3f} to {:.3f} msecs. Aborting", m_avgRoundtrip_us/1000.0, getLogName(), roundtrip_ms);
            valid = false;
        }
    }

    if (!valid)
    {
        sampleRunComplete();
        m_lock.panic();
        return;
    }

    double clientoffset_us = clientoffset_ns / 1000.0;

    if (m_initState == InitState::RUNNING)
    {
        emit signalNewOffsetMeasurement(m_name,
                                        clientoffset_us,
                                        m_offsetMeasurementHistory->getMeanAbsoluteDeviation_us(),
                                        m_offsetMeasurementHistory->getSD_us());
    }

    if (!m_averagesInitialized)
    {
        m_avgClientOffset = clientoffset_us;
        m_avgRoundtrip_us = roundtrip_us;
        m_averagesInitialized = true;
        m_prev = m_avgClientOffset;
    }
    else
    {
        m_avgClientOffset = 0.9 * m_avgClientOffset + 0.1 * clientoffset_us;
        m_avgRoundtrip_us = 0.9 * m_avgRoundtrip_us + 0.1 * roundtrip_us;
    }

    if (m_initState == InitState::RUNNING && m_offsetMeasurementHistory->size() > 1)
    {
        // experimental constants.
#ifdef VCTCXO
        const double magicnumber_zero = 300;
        const double magicnumber_antislope = 7; // 7;
        const double magicnumber_hilock_throttle = 1.25; //1.0;
        const double magicnumber_lock_throttle = 1.25;
#else
        const double magicnumber_zero = 800;
        const double magicnumber_antislope = 10000;
        const double magicnumber_hilock_throttle = 4;
        const double magicnumber_lock_throttle = 2;
#endif
        double offset_ppm = - clientoffset_us / magicnumber_zero;

        double deltatime = m_offsetMeasurementHistory->getLastTimespan_sec();
        double levelling_ppm = - (clientoffset_us - m_prev) / (deltatime * magicnumber_antislope);

        double ppm = offset_ppm + levelling_ppm;

        m_lock.errorOffset(clientoffset_us);

        if (m_lock.isHiLock())
        {
            ppm /= magicnumber_hilock_throttle;
        }
        else if (m_lock.isLock())
        {
            ppm /= magicnumber_lock_throttle;
        }

        trace->debug("{}adjusting ppm {:7.3f} (offset {:7.3f} levelling {:7.3f})",
                     getLogName(), ppm, offset_ppm, levelling_ppm);

        double ppmLimit = 0.2;
        if (m_lock.isLock())
        {
            ppmLimit = 0.1;
        }

        if (std::fabs(ppm) > ppmLimit)
        {
            double newppm = ppm >= ppmLimit ? ppmLimit : -ppmLimit;
            trace->warn("{}large ppm adjustment value {} truncated to {}", getLogName(), ppm, newppm);
            ppm = newppm;
        }

        QJsonObject json;
        json["command"] = "adjustppm";
        json["ppm_adjust"] = QString::number(ppm);
        tcpTx(json);
    }

    std::string extra = m_initState != RUNNING ? " (wait)" : "";

    m_prev = clientoffset_us;

    std::string message = fmt::format(
                WHITE "{}{:-3d} runtime {:5.1f} roundtrip_us {:5.1f} "
                      "sd_us {:5.1f} period_sec {:2} silence_sec {:2} samples {:4} avgoffs_us {:5.1f} "
                      "offset_us " YELLOW "{:5.1f}" RESET "{}",
                getLogName(), m_offsetMeasurementHistory->getCounter(), s_systemTime->getRunningTime_secs(),
                roundtrip_us,
                m_offsetMeasurementHistory->getSD_us(),
                m_lock.getMeasurementPeriodsecs(), m_lock.getInterMeasurementDelaySecs(), m_lock.getNofSamples(),
                m_avgClientOffset,
                clientoffset_us,
                extra);

    trace->info(message);

    if (g_developmentMask & DevelopmentMask::SaveClientSummaryLines)
    {
        std::string filename = fmt::format("raw_{}_client_summary.data", name());
        trace->info("dumping {}", filename);
        DataFiles::fileApp(filename, message);
    }

    trace->debug("{}cli2ser_us {:7.3f} ser2cli_us {:7.3f} spacing_sec {}",
                 getLogName(), local_us, client_us, m_lock.getInterMeasurementDelaySecs());

    if (m_udpOverruns > 10)
    {
        trace->warn("discarded {} timesamples from udp overrun", m_udpOverruns);
    }
    m_udpOverruns = 0;

    if (m_initState == InitState::PPM_MEASUREMENTS)
    {
        if (--m_initStateCounter == 0)
        {
            m_initState = InitState::CLIENT_CONFIGURING;
        }
    }

    if (m_initState == InitState::CLIENT_CONFIGURING)
    {
        int64_t client_adjustment_ns = (int64_t) client2server_ns - roundtrip_ns / 2;

        m_averagesInitialized = false;

        QJsonObject json;
        json["command"] = "adjustclock";
        double ppm = m_offsetMeasurementHistory->getPPM();

        int64_t future_ns = 0.5 * ppm * 1000.0 * m_offsetMeasurementHistory->getLastTimespan_sec();

        json["adjust_ns"] = QString::number(client_adjustment_ns + future_ns);
        json["set_ppm"] = QString::number(-ppm);

        if (VCTCXO_MODE)
        {
            int64_t wall_offset = 0;
            for (int i = 0; i < 100; ++i)
            {
                wall_offset += s_systemTime->getWallClock() - s_systemTime->getRawSystemTime();
            }
            wall_offset /= 100;
            json["wall_offset"] = QString::number(wall_offset);
        }

        tcpTx(json);

        trace->info("{}adjusting client system time with {} ns and setting ppm to {:7.3f}", getLogName(), client_adjustment_ns, ppm);

        m_offsetMeasurementHistory->reset();
        //m_offsetMeasurementHistory->enableAverages();

        m_initState = InitState::RUNNING;
    }
    else
    {
        sampleRunComplete();
    }

    QJsonObject json;
    json["name"] = m_name;
    json["command"] = "connection_info";
    json["loss"] = QString::number(measurement.packageLossPct());
    emit signalWebsocketTransmit(json);
}


void Device::timerEvent(QTimerEvent* event)
{
    int id = event->timerId();

    if (id == m_clientPingTimer)
    {
        if (--m_clientPingCounter <= 0)
        {
            m_clientPingCounter = g_serverPingPeriod / 500;
            tcpTx("ping");
        }
    }
    else if (id == m_clientActiveTimer)
    {
        if (m_clientActive)
        {
            m_clientActive = false;
            return;
        }
        clientDisconnected();
    }
    else if (id == m_sampleRunTimer)
    {
        killTimer(m_sampleRunTimer);
        m_sampleRunTimer = TIMEROFF;

        prepareSampleRun();
    }
}


void Device::tcpTx(const QJsonObject& json)
{
    if (!m_clientConnected)
    {
        return;
    }

    if (!m_tcpSocket)
    {
        trace->error("{}cant write to socket with no client", getLogName());
        return;
    }

    TcpTxPacket tx(json);

    // segfault seen here (during debugging). dunno how to avoid that.
    if (!m_tcpSocket->isWritable())
    {
        trace->warn("{}tcp socket not writable?", getLogName());
        clientDisconnected();
        return;
    }

    if (m_tcpSocket->state() != QAbstractSocket::ConnectedState)
    {
        trace->warn("{}tcp socket not in connected state ? ({})", getLogName(), m_tcpSocket->state());
        clientDisconnected();
        return;
    }

    m_tcpSocket->write(tx.getData());
    m_statusReport.packetSentOrReceived();

    m_clientPingCounter = g_serverPingPeriod / 500;
}


void Device::tcpTx(const QString& command)
{
    QJsonObject json;
    json["command"] = command;
    tcpTx(json);
}


void Device::slotTcpRx()
{
    if (!m_clientConnected)
    {
        return;
    }

    m_tcpReadBuffer += m_tcpSocket->readAll();

    while(true)
    {
        if (m_tcpReadBuffer.size() < 2)
        {
            return;
        }
        uint16_t length = *(uint16_t*) m_tcpReadBuffer.constData();
        if (m_tcpReadBuffer.size() < length + 2)
        {
            trace->debug("{}got partial {} bytes", getLogName(), m_tcpReadBuffer.size());
            return;
        }

        QByteArray deserializedjson = m_tcpReadBuffer.mid(2, length);
        m_tcpReadBuffer.remove(0, length + 2);
        bool accepted = true;
        m_statusReport.packetSentOrReceived();

        RxPacket rx(deserializedjson);

        QString command = rx.value("command");

        if (command == "ready")
        {
            int delay = m_lock.getInterMeasurementDelaySecs();

            if (delay == 0 )
            {
                prepareSampleRun();
            }
            else
            {
                timerOn(this, m_sampleRunTimer, delay * 1000);
            }

        }
        else if (command == "ping")
        {
        }
        else if (command == "forwardoffset")
        {
            bool clientValid = rx.value("valid") == "true";
            if (!clientValid)
            {
                trace->warn("{}client measurement is invalid, retrying..", getLogName());
                m_lock.panic();
                sampleRunComplete();
            }
            else
            {
                processMeasurement(rx);
            }

        }
        else if (command == "clockadjusted")
        {
            sampleRunComplete();
        }
        else
        {
            accepted = false;
        }

        if (accepted)
        {
            m_clientActive = true;
        }
        trace->trace(deserializedjson.toStdString());
    }
}


void Device::slotUdpRx()
{
    int64_t localTime = s_systemTime->getUpdatedSystemTime();
    int64_t clientTime = *((int64_t*) m_udp->receiveDatagram().data().data());

    processTimeSample(clientTime, localTime);
    m_statusReport.packetSentOrReceived();

    while (m_udp->hasPendingDatagrams())
    {
        m_udp->receiveDatagram();
        ++m_udpOverruns;
        m_statusReport.packetSentOrReceived();
    }
}


void Device::slotNewLockState(Lock::LockState lockState)
{
    QJsonObject json;
    json["name"] = m_name;
    json["command"] = "lockstateupdate";
    json["lockstate"] = Lock::toString(lockState).c_str();
    emit signalWebsocketTransmit(json);
}


void Device::getClientOffset()
{
    tcpTx("sendforwardoffset");
}


std::string Device::getStatusReport()
{
    std::string ret = fmt::format("{} {}", name(), m_statusReport.getReport());
    ret += fmt::format(" mean.abs.dev.us={:.3f}", m_offsetMeasurementHistory->getMeanAbsoluteDeviation_us());
    m_statusReport = StatusReport();
    return ret;
}


void Device::prepareSampleRun()
{
    int count = m_lock.getNofSamples();

    QJsonObject json;
    json["command"] = "running";
    json["samples"] = QString::number(count);
    tcpTx(json);

    trace->debug("{}starting sample run with {} samples and period_ms {} (slept {} secs)",
                 getLogName(), count, m_lock.getPeriodMSecs(), m_lock.getInterMeasurementDelaySecs());
    emit signalRequestSamples(m_name, count, m_lock.getPeriodMSecs());
}


void Device::sampleRunComplete()
{
    m_measurementSeries->prepareNewDataMeasurement(m_lock.getNofSamples());
    tcpTx("sampleruncomplete");
}


std::string Device::getLogName() const
{
    return fmt::format("[{:<8}] ", m_name.toStdString());
}


std::string Device::name() const
{
    return m_name.toStdString();
}


void Device::slotSendStatus()
{
    slotNewLockState(m_lock.getLockState());
}


void Device::slotClientConnected()
{
    m_clientConnected = true;
    m_tcpSocket = m_server->nextPendingConnection();
    m_tcpSocket->setSocketOption(QAbstractSocket::LowDelayOption, 1);
    m_clientAddress = QHostAddress(m_tcpSocket->peerAddress().toIPv4Address());
    m_clientTcpPort = m_tcpSocket->peerPort();

    trace->info(IMPORTANT "{}connected from {}:{}" RESET,
                getLogName(), m_clientAddress.toString().toStdString(), m_clientTcpPort);

    connect(m_tcpSocket, &QTcpSocket::readyRead, this, &Device::slotTcpRx);

    connect(m_tcpSocket, &QAbstractSocket::disconnected,
            m_tcpSocket, &QObject::deleteLater);
}


void Device::clientDisconnected()
{
    m_clientConnected = false;
    emit signalConnectionLost(m_name);
}
