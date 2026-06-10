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
#include <QLabel>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    //设置同步时间范围
    setDateTimeEditRange();

    //初始化数据库
    if(!initDatabase()){
        return;
    }

    //
    m_syncWorker = new SyncWorker(m_database->database(), this);

    //加载左侧列表
    loadRideList();

    //连接信号加载dashboard
    connect(ui->rideListWidget,&QListWidget::currentRowChanged,this,[=](int row){
        if (row < 0 || row >= m_rides.size()) return;

        showRideDashboard(m_rides[row]);
    });
    
    
    //状态栏显示同步情况
    QLabel *syncStatusbar_label=new QLabel(this);
    ui->statusbar->addWidget(syncStatusbar_label);
    //连接信号查看同步情况
    connect(m_syncWorker,&SyncWorker::progressChanged,this,[=](const QString &TypeMessage, int current, int total){
        syncStatusbar_label->setText(TypeMessage+"已同步: "+QString::number((current*100.0)/(total*1.0),'f',2)+"%");
    });

    //同步完成重新加载左侧listwidget
    connect(m_syncWorker,&SyncWorker::finished,this,[=](){
        ui->rideListWidget->clear();

        ui->syncStateLabel->setText("已同步");
        syncStatusbar_label->setText("已同步完成");
        loadRideList();
    });

    //同步失败显示信息
    connect(m_syncWorker,&SyncWorker::failed,this,[=](const QString &message){
        syncStatusbar_label->setText("同步失败: "+message);
    });
}

void MainWindow::setDateTimeEditRange()
{
    QDateTime now = QDateTime::currentDateTime();

    QDateTime earliest(QDate::currentDate().addDays(-29),QTime(0, 0, 0));

    QDateTime latest = now;

    ui->startDateTimeEdit->setMinimumDateTime(earliest);
    ui->startDateTimeEdit->setMaximumDateTime(latest);

    ui->endDateTimeEdit->setMinimumDateTime(earliest);
    ui->endDateTimeEdit->setMaximumDateTime(latest);

    // 默认同步今天
    ui->startDateTimeEdit->setDateTime(QDateTime(QDate::currentDate(), QTime(0, 0, 0)));

    ui->endDateTimeEdit->setDateTime(latest);

    auto updateEndTimeRange = [=](const QDateTime &start)
    {
        QDateTime current = QDateTime::currentDateTime();

        //结束时间的最小值
        QDateTime minEnd = start;

        //结束时间的最大值
        QDateTime maxEnd(start.date().addDays(6),QTime(23, 59, 59));

        //如果计算出的 maxEnd 超过了当前时间，则把最大值限制为当前时间。
        if (maxEnd > current)
        {
            maxEnd = current;
        }

        ui->endDateTimeEdit->setMinimumDateTime(minEnd);
        ui->endDateTimeEdit->setMaximumDateTime(maxEnd);

        if (ui->endDateTimeEdit->dateTime() < minEnd)
        {
            ui->endDateTimeEdit->setDateTime(minEnd);
        }

        if (ui->endDateTimeEdit->dateTime() > maxEnd)
        {
            ui->endDateTimeEdit->setDateTime(maxEnd);
        }
    };

    connect(ui->startDateTimeEdit,
            &QDateTimeEdit::dateTimeChanged,
            this,
            [=](const QDateTime &start)
            {
                updateEndTimeRange(start);
            });

    updateEndTimeRange(ui->startDateTimeEdit->dateTime());
}

MainWindow::~MainWindow()
{
    delete ui;
}

bool MainWindow::initDatabase()
{
    m_database=new AppDatabase(this);
    QLabel *dbStatus_lable=new QLabel(this);
    ui->statusbar->addWidget(dbStatus_lable);
    if(!m_database->open("cycling_data.db")){
        dbStatus_lable->setText(m_database->lastError());
        return false;
    }
    dbStatus_lable->setText("已打开数据库");

    return true;
}

void MainWindow::loadRideList()
{
    RideRepository repo(m_database->database());
    m_rides=repo.findAll();

    ui->rideListWidget->clear();

    for(const Ride &ride : m_rides){
        QListWidgetItem *item=new QListWidgetItem(ride.title);
        item->setData(Qt::UserRole,ride.id);
        ui->rideListWidget->addItem(item);
    }
}

void MainWindow::showRideDashboard(const Ride &ride)
{
    
    ui->dashboardTitleLabel->setText(ride.title);
    
    //时间groupbox
    ui->rideStartValueLabel->setText(ride.startTime);
    ui->rideEndValueLabel->setText(ride.endTime);
    ui->rideDurationValueLabel->setText(QString::number(ride.durationSeconds));
    
    //心率groupbox
    ui->avgHeartRateValueLabel->setText(QString::number(ride.avgHeartRate));
    ui->maxHeartRateValueLabel->setText(QString::number(ride.maxHeartRate));
    
    //血氧groupbox
    ui->avgSpo2ValueLabel->setText(QString::number(ride.avgSpo2));
    ui->minSpo2ValueLabel->setText(QString::number(ride.minSpo2));

    //里程groupbox
    ui->tripMileageValueLabel->setText(QString::number(ride.tripMileage));
    SensorDataRepository senRepo(m_database->database());
    ui->totalMileageValueLabel->setText(
        QString::number(senRepo.findLatestTotalMileageByRideId(ride.id))
    );
    
    //速度groupbox
    ui->avgSpeedValueLabel->setText(QString::number(ride.avgSpeed));
    ui->maxSpeedValueLabel->setText(QString::number(ride.maxSpeed));

    //环境groupbox
    ui->avgTemperatureValueLabel->setText(QString::number(ride.avgTemperature));
    ui->avgHumidityValueLabel->setText(QString::number(ride.avgHumidity));

    //事件groupbox
    ui->eventCountValueLabel->setText(QString::number(ride.eventCount));

}

void MainWindow::on_syncButton_clicked()
{
    qDebug()<<"-------------------------------";

    OnenetRequestConfig config={
        .productId="pmJ97H965j",
        .deviceName="dev1",
        .authorization="version=2018-10-31&res=products%2FpmJ97H965j%2Fdevices%2Fdev1&et=1806050344&method=md5&sign=MBQgX0y0Megx1%2Bt8Zb3FkQ%3D%3D"
    };
    SyncRequest request={
        .config=config,
        .start_time=ui->startDateTimeEdit->dateTime().toMSecsSinceEpoch(),
        .end_time=ui->endDateTimeEdit->dateTime().toMSecsSinceEpoch()
    };

    m_syncWorker->start(request);
}

void MainWindow::test(){
    AppDatabase *db=new AppDatabase(this);
    if (!db->open("cycling_data.db")) {
        qDebug() << "打开数据库失败:" << db->lastError();
        return;
    }

    RideRepository rideRepo(db->database());

    QVector<Ride> rides;
    rides=rideRepo.findAll();
    for(const Ride &ride : rides ){
        qDebug()<<"--------------------------------------------------------";

        qDebug()<<ride.avgHeartRate
                <<ride.avgHumidity
                <<ride.avgSpeed
                <<ride.avgSpo2
                <<ride.avgTemperature
                <<ride.createdAt
                <<ride.deviceId
                <<ride.durationSeconds
                <<ride.endTime
                <<ride.eventCount
                <<ride.id
                <<ride.maxHeartRate
                <<ride.maxSpeed
                <<ride.minSpo2
                <<ride.startTime
                <<ride.title
                <<ride.tripMileage;
    }

}