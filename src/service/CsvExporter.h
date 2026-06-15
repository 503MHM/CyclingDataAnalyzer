#pragma once 

#include <domain/SensorSample.h>
#include <QVector>
#include <QString>

class CsvExporter
{

public:
    static bool exportSamples(const QString &filePath,const QVector<SensorSample> &samples,QString *errorMessage = nullptr);

};
