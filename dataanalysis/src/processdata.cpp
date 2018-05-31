#include "processdata.h"
#include "log.h"
#include "globals.h"
#include "systemtime.h"
#include "multicast.h"
#include "offsetmeasurementhistory.h"

#include <QDir>
#include <QStringList>
#include <QFile>

SystemTime* s_systemTime = nullptr;

DataAnalyse::DataAnalyse(MeasurementSeriesBase* server_calc,
                         MeasurementSeriesBase *client_calc,
                         const QStringList &serverfiles,
                         const QStringList &clientfiles)
{
    int index = 0;

    OffsetMeasurementHistory server_offset_history;
    server_offset_history.setFlags(DevelopmentMask::AnalysisAppendToSummary);
    OffsetMeasurementHistory client_offset_history;
    client_offset_history.setFlags(DevelopmentMask::AnalysisAppendToSummary);

    for (int i = 0; i < serverfiles.size(); i++)
    {
        QString filename = QFile(serverfiles.at(i)).fileName();
        trace->info(WHITE "processing '{}'" RESET, filename.toStdString());

        {
            dataset samples = load(clientfiles.at(i));
            client_calc->prepareNewDataMeasurement();

            for (dataline datas : samples)
            {
                client_calc->add(datas.at(0), datas.at(1));
            }

            OffsetMeasurement offset_measurement = client_calc->calculate();
            client_offset_history.add(offset_measurement);
        }

        {
            dataset samples = load(serverfiles.at(i));
            server_calc->prepareNewDataMeasurement();

            for (dataline datas : samples)
            {
                server_calc->add(datas.at(0), datas.at(1));
            }

            OffsetMeasurement offset_measurement = server_calc->calculate();
            server_offset_history.add(offset_measurement);
        }

        trace->info(server_offset_history.toString());
        index++;
    };

    for (OffsetMeasurement const& offsetMeasurement : server_offset_history.getMeasurementsSummary())
    {
        trace->info(offsetMeasurement.toString());
    }

    SampleList64 delta_offset_ns;
    int64_t prev_offset;

    for (uint i = 0; i < server_offset_history.m_offsetMeasurementsSummary.size(); ++i)
    {
        auto server = server_offset_history.m_offsetMeasurementsSummary[i];

        if (i > 0)
        {
            delta_offset_ns.push_back(server.m_offset_ns - prev_offset);
        }
        prev_offset = server.m_offset_ns;
    }

    int64_t avg = MathFunc::average(delta_offset_ns);
    for (uint i = 0; i < delta_offset_ns.size(); i++)
    {
        delta_offset_ns[i] -= avg;
    }

    trace->info("standard deviation from delta server offsets {} us",
                MathFunc::standardDeviation(delta_offset_ns) / 1000.0);
}


dataset DataAnalyse::load(const QString& filename)
{
    dataset ret;
    QFile file(filename);
    file.open(QFile::ReadOnly);
    QByteArray ba = file.readAll();
    QStringList content = QString(ba).split("\n");

    for (QString line : content)
    {
        if (line.isEmpty())
            break;
        QStringList items = line.split(" ");
        uint64_t server = items[0].toLongLong();
        uint64_t client = server + items[1].toLongLong();
        dataline entry;
        entry.append(server);
        entry.append(client);
        ret.append(entry);
    }
    return ret;
}
