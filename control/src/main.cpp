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
    parser.setApplicationDescription("wifitimesync control");
    parser.addHelpOption();
    parser.addOptions({
        {"dumpraw", "let client(s) dump all offset measurements to files"},
        {"clientsummaries", "(server) dump summaries in client files"},
        {"kill", "server, 'client', all", "target"},
        {"silence", "(server) silence in secs between measurements, 0-20 or auto", "silence"},
        {"samples", "(server) number of samples per measurement, 0-1000 or auto", "samples"}});

    parser.process(app);
    trace->info("transmitting on multicast {}:{}",
         address.toString().toStdString(),
                port);

    Control control(&app, parser, address, port);
}
