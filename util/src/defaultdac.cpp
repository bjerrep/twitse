#include "defaultdac.h"
#include "globals.h"
#include "log.h"
#include <QFile>
#include <QCoreApplication>
#include <QTextStream>


uint16_t loadDefaultDAC()
{
    uint16_t dac = 0x8000;

    if (VCTCXO_MODE)
    {
        QFile file(QCoreApplication::applicationDirPath() + "/default.dac");
        if (file.open(QIODevice::ReadOnly))
        {
            dac = file.readAll().toUShort();
            trace->info("loaded vctcxo dac = {} from default.dac", dac);
        }
        else
        {
            trace->warn("couldn't open default.dac, using vctcxo dac = {}", dac);
        }
    }
    return dac;
}


void saveDefaultDAC(uint16_t dac)
{
    QFile file(QCoreApplication::applicationDirPath() + "/default.dac");
    if (file.open(QIODevice::WriteOnly))
    {
        QTextStream stream(&file);
        stream << QString::number(dac);
        trace->info("saved new default vctcxo dac = {} to default.dac", dac);
    }
    else
    {
        trace->warn("couldn't save new default vctcxo dac = {} to default.dac", dac);
    }
}
