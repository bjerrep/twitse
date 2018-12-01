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

SystemTime* s_systemTime = nullptr;

Server::Server(QCoreApplication *parent, const QHostAddress &address, uint16_t port)
    : m_parent(parent),
      m_address(address),
      m_port(port)
{
    s_systemTime = new SystemTime(true);

    connect(&m_deviceManager, &DeviceManager::signalMulticastTx, this, &Server::slotMulticastTx );
    connect(&m_deviceManager, &DeviceManager::signalIdle, this, &Server::slotIdle );

    qRegisterMetaType<UdpRxPacketPtr>("UdpRxPacketPtr");
    qRegisterMetaType<TcpRxPacketPtr>("TcpRxPacketPtr");
    qRegisterMetaType<MulticastRxPacket>("MulticastRxPacket");

    s_systemTime->reset();

    if (VCTCXO_MODE)
    {
        int64_t m_previousRealtimeDiff = SystemTime::getWallClock_ns() - s_systemTime->getRawSystemTime();
        s_systemTime->adjustSystemTime_ns(m_previousRealtimeDiff);
        int seconds = develTurbo ? 2 : 15;
        m_softPPMColdstartTimer = startTimer(seconds * TIMER_1SEC);
        m_softPPMPrevAdjustTime = s_systemTime->getRawSystemTime();
        trace->info("wait while establishing wall clock drift relative to raw clock (15sec...)");
    }
    else
    {
        startServer();
    }
}


void Server::startServer()
{
    m_multicastThread = new QThread(m_parent);
    connect(m_parent, &QCoreApplication::aboutToQuit, m_multicastThread, &QThread::quit);
    connect(m_multicastThread, &QThread::finished, m_multicastThread, &QThread::deleteLater);

    m_multicast = new Multicast("no1", m_address, m_port);
    m_multicast->moveToThread(m_multicastThread);
    connect(m_multicast, &Multicast::rx, this, &Server::multicastRx);

    m_multicastThread->start();
    m_statusReportTimer = startTimer(g_statusReport);
}


void Server::multicastRx(const MulticastRxPacket &rx)
{
    if (rx.value("from") == "server"  || ((rx.value("to") != "server" && rx.value("to") != "all")))
    {
        return;
    }

    if (rx.value("command") == "control")
    {
        executeControl(rx);
    }
    else
    {
        m_deviceManager.process(rx);
    }
}


void Server::executeControl(const MulticastRxPacket& rx)
{
    std::string action = rx.value("action").toStdString();
    trace->info("got control command {}", action);

    if (action == "kill")
    {
        trace->info("got kill, exiting..");
        QCoreApplication::quit();
    }
    else if (action == "developmentmask")
    {
        g_developmentMask = (DevelopmentMask) rx.value("developmentmask").toInt();
    }
    else if (action == "silence")
    {
        trace->info("setting period to {}", rx.value("value").toStdString());
        Lock::setFixedMeasurementSilence_sec(rx.value("value") == "auto" ? -1 : rx.value("value").toInt());
    }
    else if (action == "samples")
    {
        trace->info("setting samples to {}", rx.value("value").toStdString());
        Lock::setFixedClientSamples(rx.value("value") == "auto" ? -1 : rx.value("value").toInt());
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

        std::string dhms =
                QDateTime::fromTime_t(s_systemTime->getRunningTime_secs()).
                toLocalTime().toString("dd:hh:mm:ss").toStdString();

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

/// vctcxo mode only. Continous ppm adjustments of the realtime clock has the potential
/// of really messing things up so the corrections tries to be invisibly small. The absolute NTP
/// time is spot on, but NTP make some serious ppm adjustments to the wall clock compared to what
/// is acceptable for the corrections here, in order to operate in a stealthy fashion.
/// The primary job are not to get unstable and next to avoid the difference between the wall
/// clock and the realtime clock to drift towards infinity. After these comes the wish to actually
/// keep the offset between the clocks minimal (which it is quite poor at delivering due to the
/// small adjustments).
///
void Server::adjustRealtimeSoftPPM()
{
    int64_t now = s_systemTime->getRawSystemTime();
    int64_t elapsed = (now - m_softPPMPrevAdjustTime) / NS_IN_SEC_F;
    m_softPPMPrevAdjustTime = now;

    // Find current deviation between the raw and wall clock and express it as a drift in ppm
    double diff_sec = (s_systemTime->getRawSystemTime() - SystemTime::getWallClock_ns()) / NS_IN_SEC_F;
    // This ppm value reflects what needs to be done, its negative if the raw time is running too fast
    double ppm = - (1000000.0 * (diff_sec - m_softPPMPreviousDiff)) / elapsed;

    m_softPPMPreviousDiff = diff_sec;

    if (m_softPPMInitState == INITIALIZE_PPM)
    {
        s_systemTime->setPPM(ppm);
        trace->info("setting raw clock software ppm to {}", ppm);
        m_softPPMInitState = FIRST_ADJUSTMENT;
        return;
    }

    // the adjustment will be a component trying to zero out relative drift and a component trying
    // to get a zero absolute difference.
    double adjustment = ppm;
    adjustment -= diff_sec / 10.0;

    // ppm adjustment rate limiter. Initially allow largish adjustments to the soft ppm but
    // gradually decrease the max rate of change to 0.0015 ppm. This is just an empirical number
    // that seems to keep everything at bay.
    if (m_softPPMAdjustLimitActive)
    {
        m_softPPMAdjustLimit /= 1.5;
        if (m_softPPMAdjustLimit < 0.0015)
        {
            m_softPPMAdjustLimit = 0.0015;
            m_softPPMAdjustLimitActive = false;
            trace->info("software ppm limit set to {}", m_softPPMAdjustLimit);
        }
    }

    adjustment = qBound(-m_softPPMAdjustLimit, adjustment, m_softPPMAdjustLimit);

    if (m_softPPMInitState == FIRST_ADJUSTMENT)
    {
        m_iir.steadyState(adjustment);
        m_softPPMInitState = RUNNING;
    }

    adjustment = m_iir.filter(adjustment);

    s_systemTime->setPPM(adjustment);

    const int period_min = 15;
    if (m_softPPMLogMessagePrescaler++ % ((60 / m_softPPMAdjustPeriodSecs) * period_min) == 0)
    {
        trace->info("wall clock to raw clock skew is {:.6f} secs -> ppm {:.3f}. Adjusting with {:.6f} to {:.3f} ppm",
                    diff_sec, ppm, adjustment, s_systemTime->getPPM());
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
    else if (VCTCXO_MODE && id == m_softPPMAdjustTimer)
    {
        // Especially at coldstart wait for the initial ppm measurement to complete for
        // all connected devices
        if (m_deviceManager.allDevicesInRunningState())
        {
            adjustRealtimeSoftPPM();
        }
    }
    else if (VCTCXO_MODE && id == m_softPPMColdstartTimer)
    {
        // Establish the initial soft ppm correction between server wall clock and raw clock
        // and start listening for clients waiting to connect
        killTimer(m_softPPMColdstartTimer);
        m_softPPMAdjustTimer = startTimer(m_softPPMAdjustPeriodSecs * TIMER_1SEC);
        adjustRealtimeSoftPPM();
        startServer();
    }
    else
    {
        trace->warn("got unexpected server timer event");
    }
}
