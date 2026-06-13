#pragma once

#include "domain/Ride.h"
#include "service/AnalysisService.h"

#include <QSqlDatabase>
#include <QString>
#include <optional>

class RideRepository{

public:

    RideRepository(const QSqlDatabase &database);

    int insertRide(const Ride &ride);

    bool updateStats(int rideId,const AnalysisService::RideStats &stats);

    std::optional<Ride> findById(int id);

    QVector<Ride> findAll();

    std::optional<Ride> findByDeviceAndTime(int deviceId,const QString &startTime,const QString &endTime);

    bool deleteById(int rideId);

    QString lastError() const;

private:

    QSqlDatabase m_database;
    QString m_lastError;

};  