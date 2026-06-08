#include "SyncService.h"
#include "service/SensorSampleBuilder.h"
#include "service/EventMapper.h"

#include "database/SensorDataRepository.h"
#include "database/EventRepository.h"
SyncService::SyncService(const QSqlDatabase &database)
{
    m_database=database;
}

bool SyncService::syncPropertiesToRide(int deviceId, int rideId, const QHash<QString, QVector<OnenetPropertyPoint>> &propertyPoints)
{
    m_lastError.clear();

    SensorSampleBuilder builder;
    builder.setDeviceAndRide(deviceId,rideId);
    SensorDataRepository sensorRepo(m_database);

    for(auto it=propertyPoints.begin();it!=propertyPoints.end();it++){
        builder.addPropertyPoints(it.key(),it.value());
    }
    
    QVector<SensorSample> samples=builder.buildSamples();
    if(!sensorRepo.insertSamples(samples)){
        m_lastError=sensorRepo.lastError();
        return false;
    }

    return true;
}

bool SyncService::syncEventsToRide(int deviceId, int rideId, const QVector<OnenetEventItem> &items)
{
    m_lastError.clear();

    EventRepository eventRepo(m_database);

    for(const auto &item : items){
        RideEvent rideevent=EventMapper::toRideEvent(item,deviceId,rideId);
        if(!eventRepo.insertEvent(rideevent)){
            m_lastError=eventRepo.lastError();
            return false;
        }
    }

    return true;
}

QString SyncService::lastError() const
{
    return m_lastError;
}