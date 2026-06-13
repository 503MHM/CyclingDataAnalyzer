#include "SyncWorker.h"

#include "database/DeviceRepository.h"
#include "database/RideRepository.h"
#include "database/SensorDataRepository.h"
#include "database/RideRepository.h"
#include "database/EventRepository.h"

#include "domain/Ride.h"

#include "service/RideBuilder.h"
#include "service/SyncService.h"
#include "service/AnalysisService.h"

#include <QDateTime>
#include <QDate>
#include <QTime>
#include <QStringList>

namespace{
    const QStringList eventIdentifiers={
        "ride_start",
        "ride_end",
        "fall_warning"
    };

    const QStringList propertyIdentifiers{
        "heart_rate",
        "spo2",
        "speed",
        "temperature",
        "humidity",
        "latitude",
        "longitude",
        "trip_mileage",
        "Total_Mileage"
    };
};



SyncWorker::SyncWorker(const QSqlDatabase &database, QObject *parent)
    :QObject(parent),
    m_database(database)
{      
    
    m_expectedPropertyCount=propertyIdentifiers.size();
    m_expectedEventCount=eventIdentifiers.size();
}

void SyncWorker::start(SyncRequest request)
{
    //清理残留状态
    m_propertyPoints.clear();
    m_eventItems.clear();
    m_receivedPropertyCount = 0;
    m_receivedEventCount = 0;
    m_finished = false;
    m_deviceId = 0;

    m_request=request;

    //获取设备ID
    DeviceRepository deviceRepo(m_database);
    m_deviceId=deviceRepo.upsertDevice(m_request.config.productId,m_request.config.deviceName);
    if(m_deviceId==0){
        emit failed("[SyncWorker]设备入库失败，"+deviceRepo.lastError());
        return;
    }

    if (m_client) {
        m_client->deleteLater();
        m_client = nullptr;
    }

    m_client = new OnenetClient(this);
    m_client->setConfig(m_request.config);


    qint64 startTime=m_request.start_time;
    qint64 endTime=m_request.end_time;


    //连接信号
    connect(m_client,&OnenetClient::eventLogReceived,this,[=](const QVector<OnenetEventItem> &events, int offset, int limit){
        m_eventItems.append(events);
        m_receivedEventCount++;

        emit progressChanged("event",m_receivedEventCount,m_expectedEventCount);
        tryFinish();
    });

    connect(m_client,&OnenetClient::propertyHistoryReceived,this,[=](const QString &identifier, const QVector<OnenetPropertyPoint> &point, int offset, int limit){
        m_propertyPoints.insert(QString(identifier),point);
        m_receivedPropertyCount++;

        emit progressChanged("property",m_receivedPropertyCount,m_expectedPropertyCount);
        tryFinish();
    });

    connect(m_client, &OnenetClient::requestFailed, this,
        [=](const QString &operation, int httpStatus, int errorCode, const QString &message) {
            if (m_finished) {
                return;
            }
            m_finished = true;
            emit failed(operation + ": " + message);
        });


    //请求事件
    for(const QString &eventIdentifier : eventIdentifiers){
        m_client->queryEventLog(eventIdentifier,startTime,endTime);
    }
    
    //请求属性
    for(const QString &proIden : propertyIdentifiers){
        m_client->queryPropertyHistory(proIden,startTime,endTime);
    }

}

void SyncWorker::tryFinish()
{   
    if(m_finished){
        return;
    }

    if( m_receivedEventCount < m_expectedEventCount || 
        m_receivedPropertyCount < m_expectedPropertyCount){
        return;
    }
    
    m_finished=true;

    //根据事件返回一个骑行记录表
    QVector<Ride> rides=RideBuilder::buildRidesFromEvents(m_deviceId,m_eventItems);
    if (rides.isEmpty()) {
        emit failed("[SyncWorker] 没有完整的骑行开始/结束事件，未创建骑行记录");
        return;
    }

    //
    RideRepository rideRepo(m_database);
    SyncService syncService(m_database);
    int lastRideId = 0;
    //遍历骑行记录表做数据库相关操作
    for(auto ride:rides){
        qint64 rideStartMs=isoDateStringToMs(ride.startTime);
        qint64 rideEndMs=isoDateStringToMs(ride.endTime);

        auto ridePropertyPoints=filterPropertyPointsByTime(m_propertyPoints,rideStartMs,rideEndMs);
        auto rideEvents=filterEventByTime(m_eventItems,rideStartMs,rideEndMs);

        bool hasPropertyData = false;
        for (auto it = ridePropertyPoints.begin(); it != ridePropertyPoints.end(); ++it) {
            if (!it.value().isEmpty()) {
                hasPropertyData = true;
                break;
            }
        }
    
        if (!hasPropertyData && rideEvents.isEmpty()) {
            continue;
        }
    
        std::optional<Ride> existingRide = rideRepo.findByDeviceAndTime(
            m_deviceId,
            ride.startTime,
            ride.endTime
        );
    
        if (existingRide) {
            int oldRideId = existingRide->id;
    
            SensorDataRepository senRepo(m_database);
            if (!senRepo.deleteByRideId(oldRideId)) {
                emit failed("[SyncWorker] 删除旧传感器数据失败: " + senRepo.lastError());
                return;
            }
    
            EventRepository eventRepo(m_database);
            if (!eventRepo.deleteByRideId(oldRideId)) {
                emit failed("[SyncWorker] 删除旧事件数据失败: " + eventRepo.lastError());
                return;
            }
    
            if (!rideRepo.deleteById(oldRideId)) {
                emit failed("[SyncWorker] 删除旧骑行记录失败: " + rideRepo.lastError());
                return;
            }
        }
    
        int rideId = rideRepo.insertRide(ride);
        if (rideId == 0) {
            emit failed("[SyncWorker] 骑行记录入库失败: " + rideRepo.lastError());
            return;
        }
    
        if (!syncService.syncPropertiesToRide(m_deviceId, rideId, ridePropertyPoints)) {
            emit failed("[SyncWorker] 属性数据入库失败: " + syncService.lastError());
            return;
        }
    
        if (!syncService.syncEventsToRide(m_deviceId, rideId, rideEvents)) {
            emit failed("[SyncWorker] 事件数据入库失败: " + syncService.lastError());
            return;
        }
    
        SensorDataRepository senRepo(m_database);
        EventRepository eventRepo(m_database);
    
        QVector<SensorSample> samples = senRepo.findByRideId(rideId);
        QVector<RideEvent> events = eventRepo.findByRideId(rideId);
    
        AnalysisService::RideStats stats =
            AnalysisService::calculateRideStats(samples, events);
    
        if (!rideRepo.updateStats(rideId, stats)) {
            emit failed("[SyncWorker] 统计计算与回写失败: " + rideRepo.lastError());
            return;
        }
    
        lastRideId = rideId;
    
    }

    // bool hasPropertyData = false;
    // for (auto it = m_propertyPoints.begin(); it != m_propertyPoints.end(); ++it) {
    //     if (!it.value().isEmpty()) {
    //         hasPropertyData = true;
    //         break;
    //     }
    // }

    // bool hasEventData = !m_eventItems.isEmpty();

    // if (!hasPropertyData && !hasEventData) {
    //     emit failed("[SyncWorker] 查询时间范围内没有 OneNET 数据，未创建骑行记录");
    //     return;
    // }

    // //1.构建骑行记录
    // Ride ride=RideBuilder::buildRideFromEventsOrRange(m_deviceId,m_eventItems,m_request.start_time,m_request.end_time);

    // RideRepository rideRepo(m_database);

    // std::optional<Ride> existingRide=rideRepo.findByDeviceAndTime(m_deviceId,ride.startTime,ride.endTime);

    // //如果存在旧数据先把旧数据删了
    // if(existingRide){
    //     const int oldRideId = existingRide->id;

    //     SensorDataRepository senRepo(m_database);
    //     if (!senRepo.deleteByRideId(oldRideId)) {
    //         emit failed("[SyncWorker] 删除旧传感器数据失败: " + senRepo.lastError());
    //         return;
    //     }

    //     EventRepository eventRepo(m_database);
    //     if (!eventRepo.deleteByRideId(oldRideId)) {
    //         emit failed("[SyncWorker] 删除旧事件数据失败: " + eventRepo.lastError());
    //         return;
    //     }

    //     if (!rideRepo.deleteById(oldRideId)) {
    //         emit failed("[SyncWorker] 删除旧骑行记录失败: " + rideRepo.lastError());
    //         return;
    //     }
    // }
    

    // int rideId=rideRepo.insertRide(ride);
    // if(rideId==0){
    //     emit failed("[SyncWorker]骑行记录入库失败，"+rideRepo.lastError());
    //     return;
    // }
    
    // //2.属性数据入库
    // SyncService service(m_database);
    // bool ok=service.syncPropertiesToRide(m_deviceId,rideId,m_propertyPoints);
    // if(!ok){
    //     emit failed("[SyncWorker]属性数据入库失败"+service.lastError());
    //     return;
    // }


    // //3.事件数据入库
    // ok=service.syncEventsToRide(m_deviceId,rideId,m_eventItems);
    // if(!ok){
    //     emit failed("[SyncWorker]事件数据入库失败"+service.lastError());
    //     return;
    // }

    // //4.统计计算与回写
    // SensorDataRepository senRepo(m_database);
    // EventRepository eventRepo(m_database);
    
    // QVector<SensorSample> samples=senRepo.findByRideId(rideId);
    // QVector<RideEvent> events=eventRepo.findByRideId(rideId);
    
    // AnalysisService::RideStats stats=AnalysisService::calculateRideStats(samples,events);
    
    // //更新
    // ok=rideRepo.updateStats(rideId,stats);
    // if(!ok){
    //     emit failed("[SyncWorker]统计计算与回写失败，"+rideRepo.lastError());
    //     return;
    // }

    if (lastRideId == 0) {
        emit failed("[SyncWorker] 没有可入库的完整骑行数据");
        return;
    }

    //5.发送完成信号
    emit finished(lastRideId);
}

qint64 SyncWorker::isoDateStringToMs(const QString &timeText)
{
    return QDateTime::fromString(timeText,Qt::ISODate).toMSecsSinceEpoch();
}

QHash<QString, QVector<OnenetPropertyPoint>> SyncWorker::filterPropertyPointsByTime(const QHash<QString, QVector<OnenetPropertyPoint>> &allPoints, qint64 startMs, qint64 endMs)
{
    QHash<QString, QVector<OnenetPropertyPoint>> rangePoints;

    for(auto it=allPoints.begin(); it!=allPoints.end(); it++){
        QVector<OnenetPropertyPoint> filteredPoints;            //存放时间范围内的属性

        for(const OnenetPropertyPoint &point:it.value()){
            if(point.timestampMs>=startMs && point.timestampMs<=endMs){
                filteredPoints.append(point);
            }
        }

        rangePoints.insert(it.key(),filteredPoints);
    }


    return rangePoints;
}

QVector<OnenetEventItem> SyncWorker::filterEventByTime(const QVector<OnenetEventItem> &events, qint64 startMs, qint64 endMs)
{
    QVector<OnenetEventItem> rangeEvent;

    for(const auto &item : events){
        if(item.timestampMs>=startMs && item.timestampMs<=endMs){
            rangeEvent.append(item);
        }
    }

    return rangeEvent;
}
