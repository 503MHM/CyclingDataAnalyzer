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
#include "service/AnalysisService.h"
#include "database/RideRepository.h"

#include <qdatetime.h>
#include <qdebug.h>
#include <QtMath>
#include <QVector>

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
    int rideId=1;
    SensorDataRepository sensorRepo(db.database());
    EventRepository eventRepo(db.database());
    RideRepository rideRepo(db.database());

    auto samples = sensorRepo.findByRideId(rideId);
    auto events = eventRepo.findByRideId(rideId);

    auto stats = AnalysisService::calculateRideStats(samples, events);

    bool ok = rideRepo.updateStats(rideId, stats);
    qDebug() << ok << rideRepo.lastError();
}
