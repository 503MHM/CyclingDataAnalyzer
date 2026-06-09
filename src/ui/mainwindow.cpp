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
#include "service/RideBuilder.h"
#include "service/SyncWorker.h"


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
    //test();

    AppDatabase *db=new AppDatabase(this);
    if (!db->open("cycling_data.db")) {
        qDebug() << "打开数据库失败:" << db->lastError();
        return;
    }

    
    OnenetRequestConfig config;
    config.authorization = "version=2018-10-31&res=products%2FpmJ97H965j%2Fdevices%2Fdev1&et=1806050344&method=md5&sign=MBQgX0y0Megx1%2Bt8Zb3FkQ%3D%3D";
    config.deviceName = "dev1";
    config.productId = "pmJ97H965j";

    QDateTime start = QDateTime::fromString("2026-06-08 00:00:00", "yyyy-MM-dd hh:mm:ss");
    QDateTime end = QDateTime::fromString("2026-06-08 23:59:59", "yyyy-MM-dd hh:mm:ss");

    SyncRequest request={
        .config=config,
        // .start_time=QDateTime(QDate::currentDate().addDays(-7),QTime(0,0,0)).toMSecsSinceEpoch(),
        // .end_time=QDateTime(QDate::currentDate(),QTime(23,59,59)).toMSecsSinceEpoch()
        .start_time=start.toMSecsSinceEpoch(),
        .end_time=end.toMSecsSinceEpoch()
    };

    SyncWorker *syncWorker=new SyncWorker(db->database());
    syncWorker->start(request);
}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::test()
{
}