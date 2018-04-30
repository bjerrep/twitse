#pragma once
#include "basicoffsetmeasurement.h"
#include <QCoreApplication>
#include <QVector>

using dataline = QVector<uint64_t>;
using dataset = QVector<dataline>;

class DataAnalyse : public QObject
{
    Q_OBJECT

public:

    DataAnalyse(MeasurementSeriesBase* timingCalc,
                MeasurementSeriesBase* client_calc,
                const QStringList& serverfiles,
                const QStringList &clientfiles);

private:

    static dataset load(const QString& filename);
};
