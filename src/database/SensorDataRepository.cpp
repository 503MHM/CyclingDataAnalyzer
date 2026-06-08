#include "SensorDataRepository.h"

#include <QSqlQuery>
#include <QSqlError>


SensorDataRepository::SensorDataRepository(const QSqlDatabase &db)
{
    m_database=db;
}

bool SensorDataRepository::insertSample(const SensorSample &sample)
{
    m_lastError.clear();

    QSqlQuery query(m_database);
    query.prepare(R"(
        insert into sensor_data(
            ride_id,
            device_id,
            timestamp_ms,
            time_text,
            heart_rate,
            spo2,
            speed,
            temperature,
            humidity,
            latitude,
            longitude,
            trip_mileage,
            total_mileage,
            created_at
        )
        values(
            :ride_id,
            :device_id,
            :timestamp_ms,
            :time_text,
            :heart_rate,
            :spo2,
            :speed,
            :temperature,
            :humidity,
            :latitude,
            :longitude,
            :trip_mileage,
            :total_mileage,
            :created_at
        )
        
        )"
    );
    query.bindValue(":ride_id",sample.rideId);
    query.bindValue(":device_id",sample.deviceId);
    query.bindValue(":timestamp_ms",sample.timestampMs);
    query.bindValue(":time_text",sample.timeText);
    query.bindValue(":heart_rate",sample.heartRate);
    query.bindValue(":spo2",sample.spo2);
    query.bindValue(":speed",sample.speed);
    query.bindValue(":temperature",sample.temperature);
    query.bindValue(":humidity",sample.humidity);
    query.bindValue(":latitude",sample.latitude);
    query.bindValue(":longitude",sample.longitude);
    query.bindValue(":trip_mileage",sample.tripMileage);
    query.bindValue(":total_mileage",sample.totalMileage);
    query.bindValue(":created_at",sample.createdAt);

    if(!query.exec()){
        m_lastError=query.lastError().text();
        return false;
    }

    return true;
}

bool SensorDataRepository::insertSamples(const QVector<SensorSample> &sample)
{
    // 开启事务
    if (!m_database.transaction()) {
        qDebug() << "开启事务失败";
        return false;
    }

    for(const auto &sam : sample){
        if(!insertSample(sam)){
            m_database.rollback();
            return false;
        }
    }

    // 全部成功，提交事务
    if (!m_database.commit()) {
        qDebug() << "提交事务失败";
        return false;
    }

    return true;
}

QVector<SensorSample> SensorDataRepository::findByRideId(int rideId)
{
    m_lastError.clear();

    QSqlQuery query(m_database);
    query.prepare(R"(
        select *
        from sensor_data
        where ride_id=:ride_id    
        )"
    );
    query.bindValue(":ride_id",rideId);
    if(!query.exec()){
        m_lastError=query.lastError().text();
        return QVector<SensorSample>();
    }

    
    QVector<SensorSample> senData;
    while(query.next()){
        SensorSample sample;
        sample.id=query.value("id").toInt();
        sample.rideId=query.value("ride_id").toInt();
        sample.deviceId=query.value("device_id").toInt();
        sample.timestampMs=query.value("timestamp_ms").toLongLong();
        sample.timeText=query.value("time_text").toString();
        sample.heartRate=query.value("heart_rate").toInt();
        sample.spo2=query.value("spo2").toInt();
        sample.speed=query.value("speed").toDouble();
        sample.temperature=query.value("temperature").toDouble();
        sample.humidity=query.value("humidity").toDouble();
        sample.latitude=query.value("latitude").toDouble();
        sample.longitude=query.value("longitude").toDouble();
        sample.tripMileage=query.value("trip_mileage").toDouble();
        sample.totalMileage=query.value("total_mileage").toDouble();
        sample.createdAt=query.value("created_at").toString();
    
        senData.append(sample);
    }

    return senData;
}

QString SensorDataRepository::lastError() const
{
    return m_lastError;
}