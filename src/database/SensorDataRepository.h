#pragma once

#include "domain/SensorSample.h"
#include <QString>
#include <QSqlDatabase>
#include <QVector>

class SensorDataRepository
{
private:
    QSqlDatabase m_database;
    QString m_lastError;

public:

    SensorDataRepository(const QSqlDatabase &db);

    bool insertSample(const SensorSample &sample);
    bool insertSamples(const QVector<SensorSample> &sample);

    QVector<SensorSample> findByRideId(int rideId);

    QString lastError()const;
};
