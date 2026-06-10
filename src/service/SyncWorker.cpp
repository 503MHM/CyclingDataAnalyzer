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

    bool hasPropertyData = false;
    for (auto it = m_propertyPoints.begin(); it != m_propertyPoints.end(); ++it) {
        if (!it.value().isEmpty()) {
            hasPropertyData = true;
            break;
        }
    }

    bool hasEventData = !m_eventItems.isEmpty();

    if (!hasPropertyData && !hasEventData) {
        emit failed("[SyncWorker] 查询时间范围内没有 OneNET 数据，未创建骑行记录");
        return;
    }

    //1.构建骑行记录
    Ride ride=RideBuilder::buildRideFromEventsOrRange(m_deviceId,m_eventItems,m_request.start_time,m_request.end_time);

    RideRepository rideRepo(m_database);
    int rideId=rideRepo.insertRide(ride);
    if(rideId==0){
        emit failed("[SyncWorker]骑行记录入库失败，"+rideRepo.lastError());
        return;
    }
    
    //2.属性数据入库
    SyncService service(m_database);
    bool ok=service.syncPropertiesToRide(m_deviceId,rideId,m_propertyPoints);
    if(!ok){
        emit failed("[SyncWorker]属性数据入库失败"+service.lastError());
        return;
    }


    //3.事件数据入库
    ok=service.syncEventsToRide(m_deviceId,rideId,m_eventItems);
    if(!ok){
        emit failed("[SyncWorker]事件数据入库失败"+service.lastError());
        return;
    }

    //4.统计计算与回写
    SensorDataRepository senRepo(m_database);
    EventRepository eventRepo(m_database);
    
    QVector<SensorSample> samples=senRepo.findByRideId(rideId);
    QVector<RideEvent> events=eventRepo.findByRideId(rideId);
    
    AnalysisService::RideStats stats=AnalysisService::calculateRideStats(samples,events);
    
    //更新
    ok=rideRepo.updateStats(rideId,stats);
    if(!ok){
        emit failed("[SyncWorker]统计计算与回写失败，"+rideRepo.lastError());
        return;
    }


    //5.发送完成信号
    emit finished(rideId);
}
