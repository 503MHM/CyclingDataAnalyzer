#include "RideBuilder.h"
#include <QVector>
#include <QDateTime>


Ride RideBuilder::buildRideFromEventsOrRange(int deviceId,
    const QVector<OnenetEventItem> &events,
    qint64 rangeStartMs,
    qint64 rangeEndMs)
{
    qint64 startMs = 0;
    qint64 endMs = 0;
    QString endValueJson;

    for (const auto &event : events) {
        if (event.identifier == "ride_start") {
            if (startMs == 0 || event.timestampMs < startMs) {
                startMs = event.timestampMs;
            }
        } 
        else if (event.identifier == "ride_end") {
            if (endMs == 0 || event.timestampMs > endMs) {
                endMs = event.timestampMs;
                endValueJson = event.valueJson;
            }
        }
    }

    if (startMs == 0 || endMs == 0 || endMs <= startMs) {
        startMs = rangeStartMs;
        endMs = rangeEndMs;
    }

    Ride ride;
    ride.deviceId = deviceId;
    ride.startTime = QDateTime::fromMSecsSinceEpoch(startMs).toString(Qt::ISODate);
    ride.endTime = QDateTime::fromMSecsSinceEpoch(endMs).toString(Qt::ISODate);
    ride.durationSeconds = static_cast<int>((endMs - startMs) / 1000);
    ride.createdAt = QDateTime::currentDateTime().toString(Qt::ISODate);
    ride.title = QDateTime::fromMSecsSinceEpoch(startMs).toString("yyyy-MM-dd hh:mm 骑行");

    return ride;
}

QVector<Ride> RideBuilder::buildRidesFromEvents(int deviceId, const QVector<OnenetEventItem> &events)
{
    QVector<OnenetEventItem> sortedEvents=events;

    std::sort(sortedEvents.begin(),sortedEvents.end(),[](const OnenetEventItem &a,const OnenetEventItem &b){
        return a.timestampMs<b.timestampMs;
    });

    QVector<Ride> rides;
    qint64 currentStartMs=0;
    for(const OnenetEventItem &event : sortedEvents){
        if(event.identifier=="ride_start"){
            currentStartMs=event.timestampMs;
        }

        else if(event.identifier=="ride_end"){
            if(currentStartMs==0 || event.timestampMs<=currentStartMs) continue;

            Ride ride;
            ride.deviceId=deviceId;
            ride.startTime=QDateTime::fromMSecsSinceEpoch(currentStartMs).toString(Qt::ISODateWithMs);
            ride.endTime = QDateTime::fromMSecsSinceEpoch(event.timestampMs).toString(Qt::ISODateWithMs);
            ride.durationSeconds = static_cast<int>((event.timestampMs - currentStartMs) / 1000);
            ride.createdAt = QDateTime::currentDateTime().toString(Qt::ISODate);
            ride.title = QDateTime::fromMSecsSinceEpoch(currentStartMs).toString("yyyy-MM-dd hh:mm 骑行");
        
            rides.append(ride);
            currentStartMs=0;
        }
    }

    return rides;
}
