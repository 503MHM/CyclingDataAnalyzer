#include "EventMapper.h"

#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>



EventMapper::EventMapper()
{
}

RideEvent EventMapper::toRideEvent(const OnenetEventItem &item,int deviceId,int rideId)
{
    RideEvent event;

    event.rideId = rideId;
    event.deviceId = deviceId;
    event.timestampMs = item.timestampMs;
    event.timeText = QDateTime::fromMSecsSinceEpoch(item.timestampMs).toString(Qt::ISODate);
    event.identifier = item.identifier;
    event.name = item.name;
    event.eventType = item.eventType;
    event.valueJson = item.valueJson;
    event.createdAt = QDateTime::currentDateTime().toString(Qt::ISODate);

    QJsonDocument doc = QJsonDocument::fromJson(item.valueJson.toUtf8());
    if (doc.isObject()) {
    QJsonObject obj = doc.object();

    if (obj.contains("longitude")) {
    event.longitude = obj.value("longitude").toDouble();
    }

    if (obj.contains("latitude")) {
    event.latitude = obj.value("latitude").toDouble();
    }

    if (obj.contains("heart_rate")) {
    event.heartRate = obj.value("heart_rate").toInt();
    }

    if (obj.contains("trip_mileage")) {
    event.tripMileage = obj.value("trip_mileage").toDouble();
    }
    }

    return event;
}