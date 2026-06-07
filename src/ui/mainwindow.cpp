#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "domain/SensorSample.h"
#include "database/AppDatabase.h"
#include "database/SensorDataRepository.h"
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
    // AppDatabase db;
    // db.open("cycling_data.db");

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

}
