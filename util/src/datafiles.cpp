#include "datafiles.h"
#include "log.h"

#include <QFile>
#include <QTextStream>

void DataFiles::dumpVectors(QString name, int loop, const SampleList64 *a, const SampleList64 *b, SampleList64 *c, SampleList64 *d)
{
    name += "_" + QString::number(loop) + ".data";
    trace->info("dumping data to {}", name.toStdString());
    QFile file(name);
    if (file.open(QIODevice::WriteOnly))
    {
        QTextStream stream(&file);
        for(uint i = 0; i < a->size(); i++)
        {
            stream << QString::number(a->at(i));
            if(b)
                stream << " " << QString::number(b->at(i));
            if(c)
                stream << " " << QString::number(c->at(i));
            if(d)
                stream << " " << QString::number(d->at(i));
            stream << endl;
        }
    }
}


void DataFiles::appendValues(QString name, int64_t *a, int64_t *b, int64_t *bb, double *c, int64_t *d)
{
    name += ".data";
    trace->info("dumping data to {}", name.toStdString());
    QFile file(name);
    if (file.open(QIODevice::WriteOnly | QIODevice::Append))
    {
        QTextStream stream(&file);
        stream << QString::number(*a);
        if(b)
            stream << " " << QString::number(*b);
        if(bb)
            stream << " " << QString::number(*bb);
        if(c)
            stream << " " << QString::number(*c);
        if(d)
            stream << " " << QString::number(*d);
        stream << endl;

    }
}

void DataFiles::fileApp(QString name, double val, bool newline)
{
    name += ".data";
    QFile file(name);
    if (file.open(QIODevice::WriteOnly | QIODevice::Append))
    {
        QTextStream stream(&file);
        stream << QString::number(val);
        if(newline)
            stream << endl;
        else
            stream << " ";
    }
}

void DataFiles::fileApp(QString name, int64_t val, bool newline)
{
    name += ".data";
    QFile file(name);
    if (file.open(QIODevice::WriteOnly | QIODevice::Append))
    {
        QTextStream stream(&file);
        stream << QString::number(val);
        if(newline)
            stream << endl;
        else
            stream << " ";
    }
}

void DataFiles::fileApp(const std::string& name, const std::string& message)
{
    QFile file(QString::fromStdString(name));
    if (file.open(QIODevice::WriteOnly | QIODevice::Append))
    {
        QTextStream stream(&file);
        stream << QString::fromStdString(message);
        stream << endl;
    }
}
