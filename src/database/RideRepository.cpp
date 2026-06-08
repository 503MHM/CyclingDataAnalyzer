#include "RideRepository.h"

#include <QSqlQuery>
#include <QSqlError>
RideRepository::RideRepository(const QSqlDatabase &database)
{
    m_database=database;
}

int RideRepository::insertRide(const Ride &ride)
{
    m_lastError.clear();


    //没找到插入一条记录
    QSqlQuery query(m_database);
    query.prepare(R"(
        insert into rides(device_id,title,start_time,end_time,duration_seconds,trip_mileage,created_at)
        values(:device_id,:title,:start_time,:end_time,:duration_seconds,:trip_mileage,:created_at)
        )"
    );
    query.bindValue(":device_id",ride.deviceId);
    query.bindValue(":title",ride.title);
    query.bindValue(":start_time",ride.startTime);
    query.bindValue(":end_time",ride.endTime);
    query.bindValue(":duration_seconds",ride.durationSeconds);
    query.bindValue(":trip_mileage",ride.tripMileage);
    query.bindValue(":created_at",ride.createdAt);

    if(!query.exec()){
        m_lastError=query.lastError().text();
        return 0;
    }

    return query.lastInsertId().toInt();
}

bool RideRepository::updateStats(int rideId, const AnalysisService::RideStats &stats)
{
    m_lastError.clear();
    QSqlQuery query(m_database);
    query.prepare(R"(
         UPDATE rides
        SET
            avg_heart_rate = :avg_heart_rate,
            max_heart_rate = :max_heart_rate,
            avg_speed = :avg_speed,
            max_speed = :max_speed,
            avg_spo2 = :avg_spo2,
            min_spo2 = :min_spo2,
            avg_temperature = :avg_temperature,
            avg_humidity = :avg_humidity,
            event_count = :event_count
        WHERE id = :id
        )"
    );
    query.bindValue(":avg_heart_rate",stats.avgHeartRate);
    query.bindValue(":max_heart_rate",stats.maxHeartRate);
    query.bindValue(":avg_speed",stats.avgSpeed);
    query.bindValue(":max_speed",stats.maxSpeed);
    query.bindValue(":avg_spo2",stats.avgSpo2);
    query.bindValue(":min_spo2",stats.minSpo2);
    query.bindValue(":avg_temperature",stats.avgTemperature);
    query.bindValue(":avg_humidity",stats.avgHumidity);
    query.bindValue(":event_count",stats.eventCount);
    query.bindValue(":id",rideId);

    if(!query.exec()){
        m_lastError=query.lastError().text();
        return false;
    }

    if (query.numRowsAffected() == 0) {
        m_lastError = "ride not found";
        return false;
    }

    return true;
}

std::optional<Ride> RideRepository::findById(int id)
{

    QSqlQuery query(m_database);
    query.prepare(R"(
        select *
        from rides
        where id= :id
        )"
    );
    query.bindValue(":id",id);
    
    if(!query.exec()){
        //查询语句执行失败
        m_lastError=query.lastError().text();
        return std::nullopt;
    }
    
    if(!query.next()){
        //查询语句执行成功但没找到
        return std::nullopt;
    }
        

    //成功找到
    Ride ride;
    ride.createdAt=query.value("created_at").toString();
    ride.deviceId=query.value("device_id").toInt();
    ride.durationSeconds=query.value("duration_seconds").toInt();
    ride.endTime=query.value("end_time").toString();
    ride.id=query.value("id").toInt();
    ride.startTime=query.value("start_time").toString();
    ride.title=query.value("title").toString();
    ride.tripMileage=query.value("trip_mileage").toDouble();

    
    return ride;
}

QString RideRepository::lastError() const
{
    return m_lastError;
}
