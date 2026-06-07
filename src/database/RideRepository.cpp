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
