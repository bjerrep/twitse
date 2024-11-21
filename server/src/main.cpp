#include "server.h"
#include "log.h"
#include "globals.h"
#include "interface.h"
#include "system.h"
#include "defaultdac.h"
#include "ntpcheck.h"

#include <csignal>
#include <execinfo.h>
#include <QCoreApplication>
#include <QCommandLineParser>
#include <QLockFile>
#include <QDir>

std::shared_ptr<spdlog::logger> trace = spdlog::stdout_color_mt("console");

int g_developmentMask = DevelopmentMask::None;
int g_randomTrashPromille = 0;
QLockFile lockFile(QDir::temp().absoluteFilePath("twitse_server.lock"));


void signalHandler(int signal)
{
    void *array[30];
    int entries = backtrace(array, 30);
    trace->critical("caught signal {}", signal);
    backtrace_symbols_fd(array, entries, STDOUT_FILENO);
    exit(EXIT_FAILURE);
}


void signalCtrlCHandler(int signal)
{
    lockFile.unlock();
    QCoreApplication::exit(1);
}


void signalIgnoreSIGPIPE(int signal)
{
    printf("--------------- ignoring signal SIGPIPE ---------------");
}


int main(int argc, char *argv[])
{
    spdlog::set_level(spdlog::level::debug);
    spdlog::set_pattern(logformat);

    if(!lockFile.tryLock(0))
    {
        trace->critical("another instance of twitse_server is already running");
        exit(1);
    }

    signal(SIGSEGV, signalHandler);
    signal(SIGABRT, signalHandler);
    signal(SIGINT, signalCtrlCHandler);
    signal(SIGPIPE, signalIgnoreSIGPIPE);

    QCoreApplication app(argc, argv);

    QHostAddress address = QHostAddress(g_multicastIp);

    QCommandLineParser parser;
    if (VCTCXO_MODE)
    {
        parser.setApplicationDescription("twitse server - vctcxo build");
        I2C_Access::I2C()->writeVCTCXO_DAC(loadDefaultDAC());
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
       {"turbo", "a development speedup mode with among others fast (and poor) measurements"},
       {"trash", "in a not very structured way randomly trash a random promille of samples (integer)", "promille"}
    });
    parser.process(app);

    uint16_t port = parser.value("port").toUShort();
    if (!port)
    {
        port = g_multicastPort;
    }

    g_randomTrashPromille = parser.value("trash").toInt();
    if (g_randomTrashPromille)
    {
        trace->warn("Development: Randomly trashing {} promille of samples", g_randomTrashPromille);
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
        g_developmentMask = g_developmentMask | DevelopmentMask::SameHost;
        trace->warn("samehost mode enabled");
    }

    if (parser.isSet("turbo"))
    {
        g_developmentMask = g_developmentMask | DevelopmentMask::TurboMeasurements;
        trace->warn("turbo mode enabled");
    }

    if (VCTCXO_MODE)
        trace->info("server running in vctcxo mode");
    else
        trace->info("server running in software mode");

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

    if (VCTCXO_MODE && !parser.isSet("ntp_nowait"))
    {
        if (!ntp_check("0.arch.pool.ntp.org", 10))
        {
            trace->critical("ntp failure, bailing out");
            exit(1);
        }
        trace->info("ntp is running");
    }

    Server server(&app, address, port);
    return QCoreApplication::exec();
}
