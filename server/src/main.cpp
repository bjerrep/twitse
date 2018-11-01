#include "server.h"
#include "log.h"
#include "globals.h"
#include "interface.h"
#include "system.h"

#include <signal.h>
#include <execinfo.h>
#include <QCoreApplication>
#include <QCommandLineParser>

std::shared_ptr<spdlog::logger> trace = spdlog::stdout_color_mt("console");

DevelopmentMask g_developmentMask = DevelopmentMask::None;

void signalHandler(int signal)
{
    void *array[30];
    size_t entries = backtrace(array, 30);
    trace->critical("caught signal {}", signal);
    backtrace_symbols_fd(array, entries, STDOUT_FILENO);
    exit(EXIT_FAILURE);
}


int main(int argc, char *argv[])
{
    spdlog::set_level(spdlog::level::debug);
    spdlog::set_pattern(logformat);

    signal(SIGSEGV, signalHandler);
    signal(SIGABRT, signalHandler);

    QCoreApplication app(argc, argv);

    QHostAddress address = QHostAddress(g_multicastIp);

    QCommandLineParser parser;
    if (VCTCXO_MODE)
    {
        parser.setApplicationDescription("twitse server - vctcxo build");
    }
    else
    {
        parser.setApplicationDescription("twitse server - software build");
    }
    parser.addHelpOption();
    parser.addOptions({
                          {"port", "multicast port", "port"},
                          {"loglevel", "0:error 1:info(default) 2:debug 3:all", "loglevel"},
                          {"samehost", "both server and client runs on the same host, accept unexpected measurements"},
                          {"ntp_nowait", "(vctcxo) don't wait for ntp sync"},
                          {"turbo", "a development speedup mode with fast (and poor) measurements"}
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

    if (parser.isSet("samehost"))
    {
        g_developmentMask = (DevelopmentMask) (g_developmentMask | DevelopmentMask::SameHost);
        trace->warn("samehost mode enabled");
    }

    if (parser.isSet("turbo"))
    {
        g_developmentMask = (DevelopmentMask) (g_developmentMask | DevelopmentMask::TurboMeasurements);
        trace->warn("turbo mode enabled");
    }

#ifdef VCTCXO
    trace->info("server running in vctcxo mode");
#else
    trace->info("server running in standalone mode");
#endif

    trace->info(IMPORTANT "server starting at {} with multicast at {}:{}" RESET,
                Interface::getLocalAddress().toString().toStdString(),
                address.toString().toStdString(),
                port);

    struct sched_param param;
    param.sched_priority = 99;
    if (sched_setscheduler(0, SCHED_FIFO, &param))
    {
        trace->critical("unable to set realtime priority");
    }

    if (!parser.isSet("ntp_nowait"))
    {
        // Wait for ntp to get synced if it isn't. This is experimental, the
        // proper solution would probably be that the server didn't
        // start before ntp was up and running (?)
        //
        for (int i=1; i <= 10; i++)
        {
            if (!System::ntpSynced())
            {
                trace->info("waiting for ntp sync {}:10", i);
                sleep(1);
            }
        }
        if (!System::ntpSynced())
        {
            trace->error("giving up waiting for ntp sync");
        }
    }

    Server server(&app, address, port);
    return app.exec();
}
