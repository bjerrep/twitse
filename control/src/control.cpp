#include "control.h"
#include "log.h"
#include "globals.h"

#include <QObject>
#include <QThread>
#include <QTimer>
#include <QCoreApplication>
#include <QCommandLineParser>
#include <iostream>


Control::Control(QCoreApplication *parent, QCommandLineParser& parser, const QHostAddress &address, uint16_t port)
    : m_parent(parent)
{
    DevelopmentMask development_mask = DevelopmentMask::None;

    m_multicast = new Multicast("no1", address, port);

    int success = 0;
    if (parser.isSet("samples"))
    {
        MulticastTxPacket tx(KeyVal{
            {"from", "control"},
            {"command", "control"},
            {"to", "server"},
            {"action", "samples"},
            {"value", parser.value("samples")}});
        m_multicast->tx(tx);
        success++;
    }
    if (parser.isSet("silence"))
    {
        MulticastTxPacket tx(KeyVal{
            {"from", "control"},
            {"command", "control"},
            {"to", "server"},
            {"action", "silence"},
            {"value", parser.value("silence")}});
        m_multicast->tx(tx);
        success++;
    }
    if (parser.isSet("kill"))
    {
        MulticastTxPacket tx(KeyVal{
            {"from", "control"},
            {"command", "control"},
            {"to", parser.value("kill")},
            {"action", "kill"}});
        m_multicast->tx(tx);
        success++;
    }

    if (!success)
    {
        trace->error("nothing to do? try --help");
    }

    if (parser.isSet("dumpraw"))
    {
        development_mask = (DevelopmentMask) (development_mask | DevelopmentMask::SaveMeasurements);
        success++;
    }
    if (parser.isSet("clientsummaries"))
    {
        development_mask = (DevelopmentMask) (development_mask | DevelopmentMask::SaveClientSummaryLines);
        success++;
    }
    if (development_mask != DevelopmentMask::None)
    {
        MulticastTxPacket tx(KeyVal{
            {"from", "control"},
            {"command", "control"},
            {"to", "all"},
            {"action", "developmentmask"},
            {"developmentmask", QString::number(development_mask)}});
        m_multicast->tx(tx);
    }
}
