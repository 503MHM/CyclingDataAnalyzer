#pragma once

#include "domain/SensorSample.h"
#include "domain/RideEvent.h"

class AnalysisService
{
public:
    struct RideStats
    {
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

    static RideStats calculateRideStats(const QVector<SensorSample> &samples,
                                        const QVector<RideEvent> &events);
};
