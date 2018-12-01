#pragma once

#include "mathfunc.h"

#include <QString>

class DataFiles
{
public:
    static void dumpVectors(QString name, int loop,
                            const SampleList64 *a,
                            const SampleList64 *b = nullptr,
                            const SampleList64 *c = nullptr,
                            const SampleList64 *d = nullptr);

    static void appendValues(QString name, int64_t *a, int64_t *b = nullptr, int64_t *bb = nullptr, double *c = nullptr, int64_t *d = nullptr);

    static void fileApp(QString name, double val, bool newline = false);

    static void fileApp(QString name, int64_t val, bool newline = false);

    static void fileApp(const std::string& name, const std::string& message);

};
