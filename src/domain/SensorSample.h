#pragma once

#include <QString>

struct SensorSample{
    int id = 0;
    int rideId = 0;
    int deviceId = 0;
    qint64 timestampMs = 0;
    QString timeText;

    int heartRate = 0;
    int spo2 = 0;
    double speed = 0.0;
    double temperature = 0.0;
    double humidity = 0.0;
    double latitude = 0.0;
    double longitude = 0.0;
    double tripMileage = 0.0;
    double totalMileage = 0.0;

    QString createdAt;
};