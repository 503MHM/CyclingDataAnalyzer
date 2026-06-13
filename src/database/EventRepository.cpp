#include "EventRepository.h"

#include <qsqlquery.h>

EventRepository::EventRepository(const QSqlDatabase &db)
{
    m_database=db;
}

bool EventRepository::insertEvent(const RideEvent &event)
{
    m_lastError.clear();

    QSqlQuery query(m_database);
    query.prepare(R"(
        insert into events(
            ride_id,
            device_id,
            timestamp_ms,
            time_text,
            identifier,
            name,
            event_type,
            level,
            value_json,
            latitude,
            longitude,
            heart_rate,
            trip_mileage,
            message,
            created_at
            )
            values(
            :ride_id,
            :device_id,
            :timestamp_ms,
            :time_text,
            :identifier,
            :name,
            :event_type,
            :level,
            :value_json,
            :latitude,
            :longitude,
            :heart_rate,
            :trip_mileage,
            :message,
            :created_at
            )    
        
        )"
    );
    query.bindValue(":ride_id", event.rideId);
    query.bindValue(":device_id", event.deviceId);
    query.bindValue(":timestamp_ms", event.timestampMs);
    query.bindValue(":time_text", event.timeText);
    query.bindValue(":identifier", event.identifier);
    query.bindValue(":name", event.name);
    query.bindValue(":event_type", event.eventType);
    query.bindValue(":level", event.level);
    query.bindValue(":value_json", event.valueJson);
    query.bindValue(":latitude", event.latitude);
    query.bindValue(":longitude", event.longitude);
    query.bindValue(":heart_rate", event.heartRate);
    query.bindValue(":trip_mileage", event.tripMileage);
    query.bindValue(":message", event.message);
    query.bindValue(":created_at", event.createdAt);

    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return false;
    }

    return true;
}

QVector<RideEvent> EventRepository::findByRideId(int rideId)
{
    m_lastError.clear();

    QSqlQuery query(m_database);
    query.prepare(R"(
        select *
        from events
        where ride_id=:ride_id
        )"
    );
    query.bindValue(":ride_id",rideId);
    if(!query.exec()){
        m_lastError=query.lastError().text();
        return QVector<RideEvent>();
    }


    QVector<RideEvent> rideevent_vec;
    
    while(query.next()){
        
        RideEvent rideevent;
        rideevent.id=query.value("id").toInt();
        rideevent.rideId=query.value("ride_id").toInt();
        rideevent.deviceId=query.value("device_id").toInt();
        rideevent.timestampMs=query.value("timestamp_ms").toLongLong();
        rideevent.timeText=query.value("time_text").toString();
        rideevent.identifier=query.value("identifier").toString();
        rideevent.name=query.value("name").toString();
        rideevent.eventType=query.value("event_type").toInt();
        rideevent.level=query.value("level").toString();
        rideevent.valueJson=query.value("value_json").toString();
        rideevent.latitude=query.value("latitude").toDouble();
        rideevent.longitude=query.value("longitude").toDouble();
        rideevent.heartRate=query.value("heart_rate").toInt();
        rideevent.tripMileage=query.value("trip_mileage").toDouble();
        rideevent.message=query.value("message").toString();

        rideevent_vec.append(rideevent);
    }

    return rideevent_vec;
}

bool EventRepository::deleteByRideId(int rideId)
{
    m_lastError.clear();

    QSqlQuery query(m_database);
    query.prepare(R"(
            delete from events
            where ride_id=:ride_id
        )"
    );
    query.bindValue(":ride_id",rideId);

    if(!query.exec()){
        m_lastError=query.lastError().text();
        return false;
    }

    return true;
}

QString EventRepository::lastError() const
{
    return m_lastError;
}
