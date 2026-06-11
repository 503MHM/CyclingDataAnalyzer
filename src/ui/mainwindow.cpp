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
#include <QTableWidget>
#include <QTableWidgetItem>

#include <QtCharts/QDateTimeAxis>
#include <QtCharts/QValueAxis>
#include <QToolTip>
#include <QCursor>


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
        //设置Events页面
        showRideEvents(m_rides[row].id);
        //设置raw data页面
        showRideRawData(m_rides[row].id);
        //设置曲线页面
        showRideCharts(m_rides[row].id);
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
    
    
    //设置Event TableWidget的行为
    ui->eventsTableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);          //禁止编辑
    ui->eventsTableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);         //选中整行
    ui->eventsTableWidget->setSelectionMode(QAbstractItemView::SingleSelection);        //一次只能选中一行
    auto *header = ui->eventsTableWidget->horizontalHeader();                           //获取水平表头对象
    header->setSectionResizeMode(QHeaderView::ResizeToContents);                        //所有列的初始宽度根据内容自动调整
    header->setSectionResizeMode(4, QHeaderView::Stretch);                              //第 5 列设置为拉伸模式
    header->setMinimumSectionSize(70);                                                  //所有列的最小宽度为 70 像素
    //ui->eventsTableWidget->verticalHeader()->setVisible(false);                       //隐藏左侧的行号列


    //设置Raw tableView属性行为
    m_rawDataModel=new QSqlQueryModel(this);
    ui->rawDataTableView->setModel(m_rawDataModel);
    ui->rawDataTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->rawDataTableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->rawDataTableView->horizontalHeader()->setStretchLastSection(true);
    
    //初始化图表
    m_heartChartView=new InteractiveChartView(this);
    m_speedChartView=new InteractiveChartView(this);

    auto *layout=new QVBoxLayout(ui->chartsContainerWidget);
    layout->setContentsMargins(0,0,0,0);
    layout->setSpacing(8);
    layout->addWidget(m_heartChartView);
    layout->addWidget(m_speedChartView);

    //
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


    //设置Events页面
    //showRideEvents(ride.id);
}

void MainWindow::showRideEvents(int rideId)
{
    EventRepository eventRepo(m_database->database());
    QVector<RideEvent> events=eventRepo.findByRideId(rideId);

    ui->eventsTableWidget->setRowCount(events.size());
    
    for(int row=0;row<events.size();row++){
        RideEvent event=events[row];

        ui->eventsTableWidget->setItem(row,0,new QTableWidgetItem(event.timeText));
        ui->eventsTableWidget->setItem(row,1,new QTableWidgetItem(event.name));

        QString eventType;
        if(event.eventType==1) eventType="信息";
        else if(event.eventType==2) eventType="告警";
        else if(event.eventType==3) eventType="故障";
        ui->eventsTableWidget->setItem(row,2,new QTableWidgetItem(eventType));
        ui->eventsTableWidget->setItem(row,3,new QTableWidgetItem(event.heartRate ? QString::number(event.heartRate):"NULL"));

        QString location=QString("%1,%2").arg(event.latitude,0,'f',6).arg(event.longitude,0,'f',6);
        ui->eventsTableWidget->setItem(row,4,new QTableWidgetItem(location));
    }

}

void MainWindow::showRideRawData(int rideId)
{
    QSqlQuery query(m_database->database());
    query.prepare(R"(
        select
            time_text AS 时间,
            heart_rate AS 心率,
            spo2 AS 血氧,
            speed AS 速度,
            temperature AS 温度,
            humidity AS 湿度,
            latitude AS 纬度,
            longitude AS 经度,
            trip_mileage AS 单次里程,
            total_mileage AS 总里程
        from sensor_data
        where ride_id=:ride_id
        order by timestamp_ms asc
        )"
    );
    query.bindValue(":ride_id",rideId);

    if(!query.exec()){
        ui->statusbar->showMessage("原始数据查询失败: " + query.lastError().text(),5000);
        return;
    }

    m_rawDataModel->setQuery(query);
}

void MainWindow::showRideCharts(int rideId)
{
    SensorDataRepository senRepo(m_database->database());
    QVector<SensorSample> samples=senRepo.findByRideId(rideId);

    QLineSeries *heartSeries=new QLineSeries();
    heartSeries->setName("心率");

    QLineSeries *speedSeries=new QLineSeries();
    speedSeries->setName("速度");

    double maxHeart=0;
    double maxSpeed=0;


    for(int i=0;i<samples.size();i++){
        heartSeries->append(samples[i].timestampMs,samples[i].heartRate);
        speedSeries->append(samples[i].timestampMs,samples[i].speed);
    
        if(samples[i].heartRate>maxHeart) maxHeart=samples[i].heartRate;
        if(samples[i].speed>maxSpeed) maxSpeed=samples[i].speed;
    }

    //添加悬停显示信息
    connect(heartSeries,&QLineSeries::hovered,this,[](const QPointF &point, bool state){
        if(!state){
            QToolTip::hideText();
            return;
        }
        QString time=QDateTime::fromMSecsSinceEpoch(static_cast<qint64>(point.x())).toString("HH:mm:ss");

        QToolTip::showText(QCursor::pos(),QString("时间: %1\n心率: %2 bpm").arg(time).arg(point.y(), 0, 'f', 0));
    });

    connect(speedSeries,&QLineSeries::hovered,this,[](const QPointF &point, bool state){
        if(!state){
            QToolTip::hideText();
            return;
        }
        QString time=QDateTime::fromMSecsSinceEpoch(static_cast<qint64>(point.x())).toString("HH:mm:ss");

        QToolTip::showText(QCursor::pos(),QString("时间: %1\n速度: %2 km/h").arg(time).arg(point.y(), 0, 'f', 0));
    });




    //1.心率表
    auto *heartChart=new QChart();
    heartChart->addSeries(heartSeries);
    heartChart->setTitle("心率曲线");
    
    //创建X轴对象
    QDateTimeAxis *heartAxisX=new QDateTimeAxis();
    heartAxisX->setFormat("HH:mm:ss");              //设置轴格式
    heartAxisX->setTitleText("时间");               //设置轴名称标题

    //创建Y轴对象
    QValueAxis *heartAxisY=new QValueAxis();
    heartAxisY->setTitleText("心率(bpm)");
    heartAxisY->setRange(0,maxHeart+10);

    //将轴对象添加到图表中
    heartChart->addAxis(heartAxisX,Qt::AlignBottom);
    heartChart->addAxis(heartAxisY,Qt::AlignLeft);
    //
    heartSeries->attachAxis(heartAxisX);
    heartSeries->attachAxis(heartAxisY);

    //2.速度表
    auto *speedChart=new QChart();
    speedChart->addSeries(speedSeries);
    speedChart->setTitle("速度曲线");

    //创建X轴对象
    QDateTimeAxis *speedAxisX=new QDateTimeAxis();
    speedAxisX->setFormat("HH:mm:ss");              //设置轴格式
    speedAxisX->setTitleText("时间");               //设置轴名称标题

    //创建Y轴对象
    QValueAxis *speedAxisY=new QValueAxis();
    speedAxisY->setTitleText("速度(km/h)");
    speedAxisY->setRange(0,maxSpeed>0 ? maxSpeed+2 : 10);

    //将轴对象添加到图表中
    speedChart->addAxis(speedAxisX,Qt::AlignBottom);
    speedChart->addAxis(speedAxisY,Qt::AlignLeft);

    //
    speedSeries->attachAxis(speedAxisX);
    speedSeries->attachAxis(speedAxisY);


    m_heartChartView->setChart(heartChart);
    m_speedChartView->setChart(speedChart);
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
