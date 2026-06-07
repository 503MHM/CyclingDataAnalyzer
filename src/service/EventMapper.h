#pragma once

#include "database/EventRepository.h"
#include "network/OnenetTypes.h"


class EventMapper
{
private:

public:
    EventMapper();

    static RideEvent toRideEvent(const OnenetEventItem &item, int deviceId, int rideId);
};
