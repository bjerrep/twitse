#include "multicast.h"
#include "log.h"
#include "interface.h"
#include "globals.h"
#include "systemtime.h"

#include <signal.h>
#include <execinfo.h>
#include <QCommandLineParser>

std::shared_ptr<spdlog::logger> trace = spdlog::stdout_color_mt("console");


class Rx : public QObject
{
    Q_OBJECT

    Multicast* m_multicast;

public:
    Rx(uint16_t port)
    {
        m_multicast = new Multicast("rx", QHostAddress(g_multicastIp), port);

        // send what will act as the start marker
        connect(m_multicast, &Multicast::rx, this, &Rx::slot_rx);
    }

    ~Rx()
    {
        delete m_multicast;
    }

public slots:
    void slot_rx(MulticastRxPacketPtr rx)
    {
        int64_t local = SystemTime::getWallClock();

        if (rx->value("state") == "stop")
        {
            qApp->quit();
            return;
        }

        int64_t remote = rx->value("time").toLongLong();
        trace->info(local - remote);
    }
};


int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    QHostAddress address = QHostAddress(g_multicastIp);

    QCommandLineParser parser;
    parser.setApplicationDescription("multicast test - rx");
    parser.addHelpOption();
    parser.addOptions({
                          {"port", "multicast port", "port"},
                          {"loglevel", "0:error 1:info(default) 2:debug 3:all", "loglevel"}
                      });
    parser.process(app);

    uint16_t port = parser.value("port").toUShort();
    if (!port)
    {
        port = g_multicastPort;
    }

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

    Rx rx(port);

    return app.exec();
}

#include "receiver.moc"
