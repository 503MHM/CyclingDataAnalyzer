#pragma once 

#include <QString>

struct Ride
{
    int id = 0;
    int deviceId = 0;
    QString title;
    QString startTime;
    QString endTime;
    int durationSeconds = 0;
    double tripMileage = 0.0;
    QString createdAt;

    double avgHeartRate = 0.0;
    int maxHeartRate = 0;
    double avgSpeed = 0.0;
    double maxSpeed = 0.0;
    double avgSpo2 = 0.0;
    int minSpo2 = 0;
    double avgTemperature = 0.0;
    double avgHumidity = 0.0;
    int eventCount = 0;
};
