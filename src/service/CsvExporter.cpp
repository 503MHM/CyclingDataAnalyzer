#include "CsvExporter.h"

#include <QFile>
#include <QTextStream>

bool CsvExporter::exportSamples(const QString &filePath, const QVector<SensorSample> &samples, QString *errorMessage)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        if (errorMessage) {
            *errorMessage = file.errorString();
        }
        return false;
    }

    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);

    out << "time,heart_rate,spo2,speed,temperature,humidity,latitude,longitude,trip_mileage,total_mileage\n";
    for (const SensorSample &sample : samples) {
        out << sample.timeText << ','
            << sample.heartRate << ','
            << sample.spo2 << ','
            << sample.speed << ','
            << sample.temperature << ','
            << sample.humidity << ','
            << sample.latitude << ','
            << sample.longitude << ','
            << sample.tripMileage << ','
            << sample.totalMileage << '\n';
    }

    return true;
}
