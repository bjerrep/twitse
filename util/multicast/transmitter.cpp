#include "multicast.h"
#include "log.h"
#include "interface.h"
#include "globals.h"
#include "systemtime.h"

#include <signal.h>
#include <execinfo.h>
#include <QCommandLineParser>

std::shared_ptr<spdlog::logger> trace = spdlog::stdout_color_mt("console");


class Tx : public QObject
{
    Q_OBJECT

    Multicast* m_multicast;
    int m_nof_samples;

public:
    Tx(uint16_t port, int delay_ms, int nof_samples) : m_nof_samples(nof_samples)
    {
        m_multicast = new Multicast("tx", QHostAddress(g_multicastIp), port);

        // send what will act as the start marker
        connect(m_multicast, &Multicast::rx, this, &Tx::slot_rx);
        MulticastTxPacket tx(KeyVal{
            {"delay", QString::number(delay_ms)},
            {"samples", QString::number(nof_samples)}
            });
        m_multicast->tx(tx);
        usleep(100000);

        startTimer(delay_ms);
    }

    ~Tx()
    {
        delete m_multicast;
    }

    void timerEvent(QTimerEvent*)
    {
        trace->info("tx");
        MulticastTxPacket tx(KeyVal{
            {"time", QString::number(SystemTime::getWallClock())}
            });
        m_multicast->tx(tx);

        if (m_nof_samples-- == 0)
        {
            MulticastTxPacket tx(KeyVal{
                {"state", "stop"}
                });
            m_multicast->tx(tx);
            usleep(100000);
            m_multicast->tx(tx);
            usleep(100000);
            m_multicast->tx(tx);
            sleep(1);
            qApp->quit();
        }
    }

public slots:
    void slot_rx(MulticastRxPacketPtr)
    {
    }
};


int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    QHostAddress address = QHostAddress(g_multicastIp);

    QCommandLineParser parser;
    parser.setApplicationDescription("multicast test - tx");
    parser.addHelpOption();
    parser.addOptions({
                          {"port", "multicast port", "port"},
                          {"delay", "inter measurement delay in ms", "delay"},
                          {"samples", "nof samples", "samples"},
                          {"loglevel", "0:error 1:info(default) 2:debug 3:all", "loglevel"}
                      });
    parser.process(app);

    uint16_t port = parser.value("port").toUShort();
    if (!port)
    {
        port = g_multicastPort;
    }

    int delay_ms = parser.value("delay").toInt();

    int nof_samples = parser.value("samples").toInt();

    spdlog::level::level_enum loglevel = spdlog::level::info;
    if (!parser.value("loglevel").isEmpty())
    {
        switch (parser.value("loglevel").toInt())
        {
        case 0 : loglevel = spdlog::level::err; break;
        case 2 : loglevel = spdlog::level::debug; break;
        case 3 : loglevel = spdlog::level::trace; break;
        default : break;
        }
    }
    spdlog::set_level(loglevel);
    spdlog::set_pattern(logformat);

    struct sched_param param;
    param.sched_priority = 99;
    if (sched_setscheduler(0, SCHED_FIFO, &param))
    {
        trace->critical("unable to set realtime priority");
    }

    Tx tx(port, delay_ms, nof_samples);

    return app.exec();
}

#include "transmitter.moc"
