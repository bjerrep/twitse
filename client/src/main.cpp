#include "client.h"
#include "multicast.h"
#include "log.h"
#include "interface.h"
#include "globals.h"

#include <csignal>
#include <execinfo.h>
#include <QCommandLineParser>
#include <QLockFile>
#include <QDir>


std::shared_ptr<spdlog::logger> trace = spdlog::stdout_color_mt("console");

int g_developmentMask = DevelopmentMask::None;
QLockFile lockFile(QDir::temp().absoluteFilePath("twitse_client.lock"));

void signalHandler(int signal)
{
    void *array[100];
    int entries = backtrace(array, 100);
    trace->critical("caught signal {}", signal);
    backtrace_symbols_fd(array, entries, STDOUT_FILENO);
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[])
{
    signal(SIGSEGV, signalHandler);
    signal(SIGABRT, signalHandler);

    QCoreApplication app(argc, argv);

    QHostAddress address = QHostAddress(g_multicastIp);

    QCommandLineParser parser;
    if (VCTCXO_MODE)
    {
        parser.setApplicationDescription("twitse client - vctcxo build");
    }
    else
    {
        parser.setApplicationDescription("twitse client - software build");
    }

    parser.addHelpOption();
    parser.addOptions({
                          {"port", "multicast port", "port"},
                          {"id", "client name in the form group:device", "id"},
                          {"loglevel", "0:error 1:info(default) 2:debug 3:all", "loglevel"},
                          {"noclockadj", "Development: dont adjust the clock"},
                          // can be set at runtime with control application
                          {"fixedadjust", "Development: use a fixed ppm value", "fixedadjust"},
                          {"allowmultiple", "Development: allow multiple clients"},
                      });
    parser.process(app);

    if(!parser.isSet("allowmultiple") && !lockFile.tryLock(0))
    {
        trace->critical("another instance of twitse_client is already running (see --allowmultiple)");
        exit(1);
    }

    uint16_t port = parser.value("port").toUShort();
    if (!port)
    {
        port = g_multicastPort;
    }
    bool no_clock_adj = parser.isSet("noclockadj");

    double fixed_adjust = 0.0;
    bool use_fixed_adjust = false;
    if (!parser.value("fixedadjust").isNull())
    {
        fixed_adjust = parser.value("fixedadjust").toDouble();
        use_fixed_adjust = true;
    }

    QString id = parser.value("id");
    if (id.count(":") != 1)
    {
        trace->critical("need a unique identifier in the form group:device (--id)");
        exit(1);
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

    if (VCTCXO_MODE)
        trace->info("client " WHITE "'{}'" RESET " running in vctcxo mode", id.toStdString());
    else
        trace->info("client " WHITE "'{}'" RESET " running in software mode", id.toStdString());

    trace->info("listening on multicast {}:{} on interface {}",
                address.toString().toStdString(),
                port,
                Interface::getLocalAddress().toString().toStdString());

    struct sched_param param;
    param.sched_priority = 99;
    if (sched_setscheduler(0, SCHED_FIFO, &param))
    {
        trace->critical("unable to set realtime priority");
    }

    Client client(&app, id, address, port, loglevel, no_clock_adj, !use_fixed_adjust, fixed_adjust);

    return QCoreApplication::exec();
}
