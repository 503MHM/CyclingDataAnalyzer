#pragma once

#include <QString>

struct RideEvent
{
    int id = 0;
    int rideId = 0;
    int deviceId = 0;
    qint64 timestampMs = 0;
    QString timeText;

    QString identifier;
    QString name;
    int eventType = 0;
    QString level;
    QString valueJson;

    double latitude = 0.0;
    double longitude = 0.0;
    int heartRate = 0;
    double tripMileage = 0.0;
    QString message;

    QString createdAt;

};
