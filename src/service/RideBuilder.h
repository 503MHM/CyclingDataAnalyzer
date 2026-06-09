#pragma once

#include "network/OnenetTypes.h"
#include "domain/Ride.h"


class RideBuilder
{

public:
    static Ride buildRideFromEventsOrRange(
        int deviceId,
        const QVector<OnenetEventItem> &events,
        qint64 rangeStartMs,
        qint64 rangeEndMs
    );


};