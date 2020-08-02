#include "control.h"
#include "log.h"
#include "globals.h"

#include <QCoreApplication>


std::shared_ptr<spdlog::logger> trace = spdlog::stdout_color_mt("console");

int main(int argc, char *argv[])
{
    spdlog::set_level(spdlog::level::info);
    spdlog::set_pattern("%v");

    QCoreApplication app(argc, argv);

    QHostAddress address = QHostAddress(g_multicastIp);
    uint16_t port = g_multicastPort;

    QCommandLineParser parser;
    parser.setApplicationDescription("twitse control");
    parser.addHelpOption();
    parser.addOptions({
        {"dumpraw", "let client(s) dump all offset samples in individual measurement files"},
        {"dumprawsingle", "as dumpraw but only the next measurement"},
        {"clientsummaries", "(server) dump measurement summaries in client files. Used for plot data."},
        {"summaryonbailingout", "(client) write measurements to file if bailing out"},
        {"sampleperiodsweep", "sweep from 5ms sample period and up for router analysis"},
        {"kill", "exit the proces(es) matching server, 'clientname' or all,"\
                 "after which systemd will default restart them again", "target"},
        {"silence", "(server) silence in secs between measurements, 0-20 or auto", "silence"},
        {"samples", "(server) number of samples per measurement, 0-1000 or auto", "samples"},
        {"vctcxodac", "(client) set the vctcxo dac to fixed value 0-65535 or auto", "vctcxodac"},
        {"client", "name of the client (for entries starting with '(client)')", "client"}});

    parser.process(app);

    trace->info("transmitting on multicast {}:{}", address.toString().toStdString(), port);

    Control control(&app, parser, address, port);
}
