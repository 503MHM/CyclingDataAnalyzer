#pragma once

#include "domain/Ride.h"

#include <QSqlDatabase>
#include <QString>
#include <optional>

class RideRepository{

public:

    RideRepository(const QSqlDatabase &database);

    int insertRide(const Ride &ride);

    std::optional<Ride> findById(int id);

    QString lastError() const;

private:

    QSqlDatabase m_database;
    QString m_lastError;

};  