#pragma once

#include "domain/RideEvent.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QSqlDatabase>

class EventRepository
{
private:

    QSqlDatabase m_database;
    QString m_lastError;

public:
    EventRepository(const QSqlDatabase &db);

    bool insertEvent(const RideEvent &event);

    QVector<RideEvent> findByRideId(int rideId);

    bool deleteByRideId(int rideId);

    QString lastError()const;

};
