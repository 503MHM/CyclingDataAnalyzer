#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "domain/SensorSample.h"
#include "database/AppDatabase.h"
#include "database/SensorDataRepository.h"
#include "domain/RideEvent.h"
#include "database/EventRepository.h"
#include "domain/RideEvent.h"
#include "network/OnenetTypes.h"
#include "service/EventMapper.h"
#include "service/SensorSampleBuilder.h"

#include <qdatetime.h>



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
    AppDatabase db;
    db.open("cycling_data.db");

    // SensorSample sample;
    // sample.rideId =1;
    // sample.deviceId =1;
    // sample.timestampMs = QDateTime::currentMSecsSinceEpoch();
    // sample.timeText = QDateTime::currentDateTime().toString(Qt::ISODate);
    // sample.heartRate = 88;
    // sample.speed = 3.2;
    // sample.createdAt = QDateTime::currentDateTime().toString(Qt::ISODate);

    
    // SensorDataRepository sensorRepo(db.database());
    // bool ok = sensorRepo.insertSample(sample);
    // qDebug() << ok << sensorRepo.lastError();


    // RideEvent event;
    // event.rideId = 1;
    // event.deviceId = 1;
    // event.timestampMs = QDateTime::currentMSecsSinceEpoch();
    // event.timeText = QDateTime::currentDateTime().toString(Qt::ISODate);
    // event.identifier = "fall_warning";
    // event.name = "摔倒告警";
    // event.eventType = 2;
    // event.level = "warning";
    // event.valueJson = R"({"longitude":113.93456,"latitude":22.54321,"heart_rate":92})";
    // event.latitude = 22.54321;
    // event.longitude = 113.93456;
    // event.heartRate = 92;
    // event.message = "检测到用户摔倒";
    // event.createdAt = QDateTime::currentDateTime().toString(Qt::ISODate);

    // EventRepository eventRepo(db.database());
    // bool ok = eventRepo.insertEvent(event);
    // qDebug() << ok << eventRepo.lastError();

//     OnenetEventItem item;
// item.timestampMs = QDateTime::currentMSecsSinceEpoch();
// item.identifier = "fall_warning";
// item.name = "摔倒告警";
// item.eventType = 2;
// item.valueJson = R"({"longitude":113.93456,"latitude":22.54321,"heart_rate":92})";

// RideEvent event = EventMapper::toRideEvent(item, 1, 1);

// EventRepository eventRepo(db.database());
// bool ok = eventRepo.insertEvent(event);
// qDebug() << ok << eventRepo.lastError();

    SensorSampleBuilder builder;
    builder.setDeviceAndRide(1,1);

    QVector<OnenetPropertyPoint> heartRatePoints;
    heartRatePoints.append({1000, "88"});
    heartRatePoints.append({2000, "90"});

    QVector<OnenetPropertyPoint> speedPoints;
    speedPoints.append({1100, "3.2"});
    speedPoints.append({2100, "3.5"});

    builder.addPropertyPoints("heart_rate",heartRatePoints);
    builder.addPropertyPoints("speed",speedPoints);

    QVector<SensorSample> samples=builder.buildSamples();
    for(const auto &sample : samples){
        qDebug()<<sample.timestampMs<<sample.heartRate<<sample.speed;
    }
}
