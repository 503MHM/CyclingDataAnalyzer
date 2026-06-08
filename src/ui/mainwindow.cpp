#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "database/AppDatabase.h"
#include "database/SensorDataRepository.h"
#include "database/EventRepository.h"
#include "database/RideRepository.h"
#include "database/DeviceRepository.h"

#include "domain/SensorSample.h"
#include "domain/RideEvent.h"
#include "domain/RideEvent.h"
#include "domain/Ride.h"

#include "service/EventMapper.h"
#include "service/SensorSampleBuilder.h"
#include "service/AnalysisService.h"
#include "service/SyncService.h"

#include "network/OnenetClient.h"
#include "network/OnenetTypes.h"

#include <qdatetime.h>
#include <qdebug.h>
#include <QtMath>
#include <QVector>
#include <QHash>
#include <functional>
#include <memory>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    test();
}

MainWindow::~MainWindow()
{
    delete ui;
}
void MainWindow::test()
{
    auto db = std::make_shared<AppDatabase>();
    if (!db->open("cycling_data.db")) {
        qDebug() << "打开数据库失败:" << db->lastError();
        return;
    }

    OnenetRequestConfig config;
    config.authorization = "version=2018-10-31&res=products%2FpmJ97H965j%2Fdevices%2Fdev1&et=1806050344&method=md5&sign=MBQgX0y0Megx1%2Bt8Zb3FkQ%3D%3D";
    config.deviceName = "dev1";
    config.productId = "pmJ97H965j";

    QDateTime dt = QDateTime::fromString("2026-06-08 00:00:00", "yyyy-MM-dd hh:mm:ss");
    QDateTime dt2 = QDateTime::fromString("2026-06-08 23:59:59", "yyyy-MM-dd hh:mm:ss");

    qint64 start_time = dt.toMSecsSinceEpoch();
    qint64 end_time = dt2.toMSecsSinceEpoch();

    QSqlDatabase sqldb = db->database();

    DeviceRepository deviceRepo(sqldb);
    int deviceId = deviceRepo.upsertDevice(config.productId, config.deviceName);

    if (deviceId == 0) {
        qDebug() << "设备入库失败:" << deviceRepo.lastError();
        return;
    }

    RideRepository rideRepo(sqldb);

    Ride ride;
    ride.deviceId = deviceId;
    ride.startTime = dt.toString(Qt::ISODate);
    ride.endTime = dt2.toString(Qt::ISODate);
    ride.createdAt = QDateTime::currentDateTime().toString(Qt::ISODate);

    int rideId = rideRepo.insertRide(ride);

    if (rideId == 0) {
        qDebug() << "骑行记录入库失败:" << rideRepo.lastError();
        return;
    }

    qDebug() << "deviceId:" << deviceId;
    qDebug() << "rideId:" << rideId;

    auto client = new OnenetClient(this);
    client->setConfig(config);

    QStringList identifiers = {
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

    auto propertyPoints = std::make_shared<QHash<QString, QVector<OnenetPropertyPoint>>>();
    auto eventItems = std::make_shared<QVector<OnenetEventItem>>();

    auto receivedPropertyCount = std::make_shared<int>(0);
    auto expectedPropertyCount = std::make_shared<int>(identifiers.size());
    auto eventDone = std::make_shared<bool>(false);
    auto finished = std::make_shared<bool>(false);

    auto tryFinish = std::make_shared<std::function<void()>>();

    *tryFinish = [=]() {
        Q_UNUSED(db);

        if (*finished) {
            return;
        }

        if (*receivedPropertyCount < *expectedPropertyCount || !*eventDone) {
            return;
        }

        *finished = true;

        SyncService syncService(sqldb);

        if (!syncService.syncPropertiesToRide(deviceId, rideId, *propertyPoints)) {
            qDebug() << "属性入库失败:" << syncService.lastError();
            return;
        }

        if (!syncService.syncEventsToRide(deviceId, rideId, *eventItems)) {
            qDebug() << "事件入库失败:" << syncService.lastError();
            return;
        }

        SensorDataRepository sensorRepo(sqldb);
        EventRepository eventRepo(sqldb);
        RideRepository rideRepo(sqldb);

        QVector<SensorSample> samples = sensorRepo.findByRideId(rideId);
        QVector<RideEvent> events = eventRepo.findByRideId(rideId);

        qDebug() << "samples count:" << samples.count();
        qDebug() << "events count:" << events.count();

        AnalysisService::RideStats stats =
            AnalysisService::calculateRideStats(samples, events);

        if (!rideRepo.updateStats(rideId, stats)) {
            qDebug() << "更新统计失败:" << rideRepo.lastError();
            return;
        }

        qDebug() << "完整测试成功";
        qDebug() << "avgHeartRate:" << stats.avgHeartRate;
        qDebug() << "maxHeartRate:" << stats.maxHeartRate;
        qDebug() << "avgSpo2:" << stats.avgSpo2;
        qDebug() << "minSpo2:" << stats.minSpo2;
        qDebug() << "avgSpeed:" << stats.avgSpeed;
        qDebug() << "maxSpeed:" << stats.maxSpeed;
        qDebug() << "avgTemperature:" << stats.avgTemperature;
        qDebug() << "avgHumidity:" << stats.avgHumidity;
        qDebug() << "eventCount:" << stats.eventCount;

        if (!samples.isEmpty()) {
            const SensorSample &sample = samples.first();
            qDebug() << "first sample:";
            qDebug() << "heartRate:" << sample.heartRate;
            qDebug() << "spo2:" << sample.spo2;
            qDebug() << "speed:" << sample.speed;
            qDebug() << "temperature:" << sample.temperature;
            qDebug() << "humidity:" << sample.humidity;
            qDebug() << "latitude:" << sample.latitude;
            qDebug() << "longitude:" << sample.longitude;
            qDebug() << "tripMileage:" << sample.tripMileage;
            qDebug() << "totalMileage:" << sample.totalMileage;
        }
    };

    connect(client, &OnenetClient::propertyHistoryReceived,
            this,
            [=](const QString &identifier,
                const QVector<OnenetPropertyPoint> &points,
                int offset,
                int limit) {
                Q_UNUSED(offset);
                Q_UNUSED(limit);

                propertyPoints->insert(identifier, points);
                (*receivedPropertyCount)++;

                qDebug() << "属性返回:"
                         << identifier
                         << "count:" << points.count()
                         << "进度:" << *receivedPropertyCount << "/" << *expectedPropertyCount;

                (*tryFinish)();
            });

    connect(client, &OnenetClient::eventLogReceived,
            this,
            [=](const QVector<OnenetEventItem> &events,
                int offset,
                int limit) {
                Q_UNUSED(offset);
                Q_UNUSED(limit);

                *eventItems = events;
                *eventDone = true;

                qDebug() << "事件返回 count:" << events.count();

                (*tryFinish)();
            });

    connect(client, &OnenetClient::requestFailed,
            this,
            [](const QString &operation,
               int httpStatus,
               int errorCode,
               const QString &message) {
                qDebug() << "请求失败:"
                         << operation
                         << "http:" << httpStatus
                         << "error:" << errorCode
                         << message;
            });

    for (const QString &identifier : identifiers) {
        client->queryPropertyHistory(identifier, start_time, end_time);
    }

    client->queryEventLog("fall_warning", start_time, end_time);
}