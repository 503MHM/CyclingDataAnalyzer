#pragma once

#include "domain/RideEvent.h"
#include "network/OnenetTypes.h"


class EventMapper
{
private:

public:
    EventMapper();

    static RideEvent toRideEvent(const OnenetEventItem &item, int deviceId, int rideId);
};
