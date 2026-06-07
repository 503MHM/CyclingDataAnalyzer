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
};
