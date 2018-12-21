#include "processdata.h"
#include "basicoffsetmeasurement.h"
#include "log.h"
#include "globals.h"
#include "basicoffsetmeasurement.h"

#include <QProcess>
#include <QDir>

int g_developmentMask = DevelopmentMask::None;

std::shared_ptr<spdlog::logger> trace = spdlog::stdout_color_mt("console");

int main(int argc, char *argv[])
{
    spdlog::set_level(spdlog::level::debug);

    QCoreApplication app(argc, argv);

    g_developmentMask = DevelopmentMask::AnalysisAppendToSummary;

    // get naturally sorted list
    QProcess ls;
    ls.start("sh -c \"ls ../../dataanalysis/data/1hour_throttle_on/server/raw_clienttime* -v1\"");
    ls.waitForFinished();
    QStringList serverfiles = QString(ls.readAll()).split("\n", QString::SkipEmptyParts);

    if (serverfiles.empty())
    {
        trace->critical("no files found?");
        exit(1);
    }

    ls.start("sh -c \"ls ../../dataanalysis/data/1hour_throttle_on/client/raw_clienttime* -v1\"");
    ls.waitForFinished();
    QStringList clientfiles = QString(ls.readAll()).split("\n", QString::SkipEmptyParts);

    if (serverfiles.size() != clientfiles.size())
    {
        trace->critical("inconsistent data set sizes for client vs server");
        exit(1);
    }

    trace->info("Analysing {} containing {} files", QDir::currentPath().toStdString(), serverfiles.size());

    MeasurementSeriesBase* server_timing = new BasicMeasurementSeries("server", MeasurementSeriesBase::LOWEST_VALUES);
    MeasurementSeriesBase* client_timing = new BasicMeasurementSeries("client", MeasurementSeriesBase::LOWEST_VALUES);

    DataAnalyse data(server_timing, client_timing, serverfiles, clientfiles);
    trace->info("exit");
}
