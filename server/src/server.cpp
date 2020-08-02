#include "server.h"
#include "defaultdac.h"
#include "log.h"
#include "globals.h"
#include "mathfunc.h"
#include "system.h"
#include "systemtime.h"
#include "device.h"
#include "apputils.h"
#include "i2c_access.h"
#include "websocket.h"

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

    qRegisterMetaType<UdpRxPacketPtr>("UdpRxPacketPtr");
    qRegisterMetaType<TcpRxPacketPtr>("TcpRxPacketPtr");
    qRegisterMetaType<MulticastRxPacket>("MulticastRxPacket");

    if (VCTCXO_MODE)
    {
        int64_t m_previousRealtimeDiff = SystemTime::getWallClock_ns() - s_systemTime->getRawSystemTime_ns();
        s_systemTime->adjustSystemTime_ns(m_previousRealtimeDiff);
        int seconds = develTurbo ? 2 : 15;
        m_wallAdjustColdstartTimer = startTimer(seconds * TIMER_1SEC);
        trace->info("wait while establishing wall clock drift relative to raw clock (15sec...)");
    }
    else
    {
        startServer();
    }
    s_systemTime->reset();
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

    m_deviceManager.initialize();
    connect(&m_deviceManager, &DeviceManager::signalMulticastTx, this, &Server::slotMulticastTx );
    connect(&m_deviceManager, &DeviceManager::signalIdle, this, &Server::slotIdle );
}


void Server::multicastRx(const MulticastRxPacket &rx)
{
    if (rx.value("from") == "server" || ((rx.value("to") != "server" && rx.value("to") != "all")))
    {
        return;
    }

    if (rx.value("command") == "control")
    {
        executeControl(rx);
    }
    else if (rx.value("command") == "metric")
    {
        processMetric(rx);
    }
    else
    {
        m_deviceManager.process(rx);
    }
}

/// Process actions sent from the standalone 'control' application. This is typically
/// something like 'kill' or entries for manipulating the server state at runtime during development.
///
void Server::executeControl(const MulticastRxPacket& rx)
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


/// Process metrics sent from clients. This will currently originate in the web department.
///
void Server::processMetric(const MulticastRxPacket& rx)
{
    std::string action = rx.value("action").toStdString();
    trace->trace("got metric {}", action);

    if (action == "vctcxo_dac")
    {
        m_deviceManager.sendVctcxoDac(rx.value("from"), rx.value("value"));
    }
    else
    {
        trace->warn("metric command not recognized, {}", action);
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
    if (m_pendingStatusReport)
    {
        m_pendingStatusReport = false;
        printStatusReport();
    }
}


void Server::printStatusReport()
{
    std::string dhms =
            QDateTime::fromTime_t(s_systemTime->getRunningTime_secs()).
            toLocalTime().toString("dd:hh:mm:ss").toStdString();

    double wall_diff_sec = (s_systemTime->getRawSystemTime_ns() - SystemTime::getWallClock_ns()) / NS_IN_SEC_F;

    trace->info(CYAN "    runtime {}, cputemp {:.1f}, clients {}, wall offset {:.6f} secs" RESET,
                dhms,
                System::cpuTemperature(),
                m_deviceManager.getDevices().size(),
                wall_diff_sec);
    const DeviceDeque& devices = m_deviceManager.getDevices();
    for (Device* device : devices)
    {
        trace->info(CYAN "      {}" RESET, device->getStatusReport());
    }
}


/// vctcxo mode only. Continous adjustments of the realtime/vctcxo clock has the potential
/// of really messing things up so the corrections should ideally be invisibly small.
/// The primary job are not to get unstable and next to avoid the difference between the wall
/// clock and the realtime clock to drift towards infinity. After these comes the wish to actually
/// keep the offset between the clocks minimal (which it is quite poor at delivering due to the
/// small adjustments).
/// Note that adjustments will not be visible before ntp decides to do something which might take
/// quite some time. Until then all clocks are equally affected when the X1 clock is adjusted.
///
void Server::adjustRealtimeSoftPPM()
{
    // Find the current absolute clock error
    double clock_offset_sec = (s_systemTime->getRawSystemTime_ns() - SystemTime::getWallClock_ns()) / NS_IN_SEC_F;
    double abs_clock_offset_sec = std::fabs(clock_offset_sec);

    int adjustment_limit = 1;
    std::string range("fine");

    // Set the update frequency and the corresponding maximum adjustment based on the time offset above.
    // The first block will be the panic mode which will most likely have a noticable impact on
    // the general twitse quality.
    // In case the wall clock is voilently off it wont be detected but the operation will remain in
    // 'panic' mode which is kind of bad.
    if (abs_clock_offset_sec > 0.5)
    {
        range = "coarse";
        adjustment_limit = 2;
    }
    else if (abs_clock_offset_sec > 0.05)
    {
        range = "fine";
        adjustment_limit = 1;
    }
    else
    {
        range = "lock";
        adjustment_limit = 0;
    }

    int LSB = - qBound(-adjustment_limit, (int) (clock_offset_sec * 1000.0), adjustment_limit);

    if (LSB)
    {
        bool success = I2C_Access::I2C()->adjustVCTCXO_DAC(LSB);
        if (!success)
        {
            trace->error("vctcxo dac is saturated at {}", LSB);
        }
    }
    trace->warn("NTP '{}'. WallClockDev={:.3f} ms LSBADJ={} DAC={}",
                range, clock_offset_sec*1000.0, LSB, I2C_Access::I2C()->getVCTCXO_DAC());
}


void Server::timerEvent(QTimerEvent* timerEvent)
{
    int id = timerEvent->timerId();

    if (id == m_statusReportTimer)
    {
        if (m_deviceManager.idle())
        {
            printStatusReport();
        }
        else
        {
            m_pendingStatusReport = true;
        }
    }
    else if (VCTCXO_MODE && id == m_wallAdjustColdstartTimer)
    {
        // Establish the initial soft ppm correction between server wall clock and raw clock
        // and start listening for clients waiting to connect
        timerOff(this, m_wallAdjustColdstartTimer);
        m_wallAdjustTimer = startTimer(m_wallAdjustPeriodSecs * TIMER_1SEC);
        adjustRealtimeSoftPPM();
        startServer();
        m_wallSaveNewDefaultDAC = startTimer(TIMER_ONE_DAY);
    }
    else if (VCTCXO_MODE && id == m_wallAdjustTimer)
    {
        // At coldstart wait for the initial ppm measurement to complete for
        // all connected devices
        if (m_deviceManager.allDevicesInRunningState())
        {
            adjustRealtimeSoftPPM();
        }
    }
    else if (VCTCXO_MODE && id == m_wallSaveNewDefaultDAC)
    {
        uint16_t dac = I2C_Access::I2C()->getVCTCXO_DAC();
        trace->info("saving the current vctcxo dac {} (daily)", dac);
        saveDefaultDAC(dac);
    }
    else
    {
        trace->warn("got unexpected server timer event");
    }
}
