#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "ui/SettingDialog.h"

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
#include "service/CsvExporter.h"


#include "network/OnenetClient.h"
#include "network/OnenetTypes.h"

#include <qdatetime.h>
#include <qdebug.h>
#include <QtMath>
#include <QVector>
#include <QHash>
#include <functional>
#include <QLabel>
#include <QTableWidget>
#include <QTableWidgetItem>

#include <QtCharts/QDateTimeAxis>
#include <QtCharts/QValueAxis>
#include <QToolTip>
#include <QCursor>
#include <QDir>
#include <QFileDialog>
#include <QSettings>
#include <QCalendarWidget>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setIcon();
    QSettings settings=SettingDialog::appSettings();
    m_rawDataPageSize=settings.value("rawData/pageSize",20).toInt();

    m_trackView=new TrackView(this);
    ui->trackLayout->addWidget(m_trackView);

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

    //连接信号
    connect(ui->rideListWidget,&QListWidget::currentRowChanged,this,[=](int row){
        if (row < 0 || row >= m_rides.size()) return;
        m_currentRideId=m_rides[row].id;
        m_rawDataPageIndex=0;
        m_rawDataTotalRows=0;
        
        //加载dashboard
        showRideDashboard(m_rides[row]);
        
        loadCurrentTab();
    });

    connect(ui->contentTabWidget,&QTabWidget::currentChanged,this,[=](){
        loadCurrentTab();
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

        ui->syncButton->setEnabled(true);
        ui->syncStateLabel->setText("已同步");
        syncStatusbar_label->setText("已同步完成");
        loadRideList();
    });

    //同步失败显示信息
    connect(m_syncWorker,&SyncWorker::failed,this,[=](const QString &message){
        ui->syncButton->setEnabled(true);
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

    connect(ui->rawPrevButton, &QPushButton::clicked, this, [=](){
        if (m_rawDataPageIndex <= 0 || m_currentRideId <= 0) return;
        m_rawDataPageIndex--;
        showRideRawData(m_currentRideId);
    });

    connect(ui->rawNextButton, &QPushButton::clicked, this, [=](){
        int pageCount = (m_rawDataTotalRows + m_rawDataPageSize - 1) / m_rawDataPageSize;
        if (pageCount <= 0) {
            pageCount = 1;
        }
        if (m_rawDataPageIndex + 1 >= pageCount || m_currentRideId <= 0) return;
        m_rawDataPageIndex++;
        showRideRawData(m_currentRideId);
    });
    
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
    latest.setTime(QTime(23,59,59));

    ui->startDateTimeEdit->setMinimumDateTime(earliest);
    ui->startDateTimeEdit->setMaximumDateTime(latest);

    ui->endDateTimeEdit->setMinimumDateTime(earliest);
    ui->endDateTimeEdit->setMaximumDateTime(latest);

    ui->startDateTimeEdit->calendarWidget()->setMinimumDate(earliest.date());
    ui->startDateTimeEdit->calendarWidget()->setMaximumDate(latest.date());

    ui->endDateTimeEdit->calendarWidget()->setMinimumDate(earliest.date());
    ui->endDateTimeEdit->calendarWidget()->setMaximumDate(latest.date());

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
            current.setTime(QTime(23,59,59));
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

    connect(ui->startDateTimeEdit,&QDateTimeEdit::dateTimeChanged,this,[=](const QDateTime &start){
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

    QString dbDir=QCoreApplication::applicationDirPath()+"/database";
    if(!QDir().mkpath(dbDir)){
        dbStatus_lable->setText("创建数据目录失败: " + dbDir);
        return false;
    }
    
    if(!m_database->open(dbDir+"/cycling_data.db")){
        dbStatus_lable->setText(m_database->lastError());
        return false;
    }
    dbStatus_lable->setText("已连接数据库");

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
    ui->rideStartValueLabel->setText(ride.startTime.section('.', 0, 0));
    ui->rideEndValueLabel->setText(ride.endTime.section('.', 0, 0));
    int secs = ride.durationSeconds;
    ui->rideDurationValueLabel->setText(
        QString("%1:%2:%3").arg(secs / 3600, 2, 10, QChar('0'))
                           .arg((secs % 3600) / 60, 2, 10, QChar('0'))
                           .arg(secs % 60, 2, 10, QChar('0')));

    //心率groupbox
    ui->avgHeartRateValueLabel->setText(QString::number(ride.avgHeartRate,'f',1));
    ui->maxHeartRateValueLabel->setText(QString::number(ride.maxHeartRate));
    
    //血氧groupbox
    ui->avgSpo2ValueLabel->setText(QString::number(ride.avgSpo2,'f',1));
    ui->minSpo2ValueLabel->setText(QString::number(ride.minSpo2));

    //里程groupbox
    SensorDataRepository senRepo(m_database->database());
    ui->tripMileageValueLabel->setText(QString::number(senRepo.findLatestTripMileageByRideId(ride.id),'f',2));
    ui->totalMileageValueLabel->setText(QString::number(senRepo.findLatestTotalMileageByRideId(ride.id),'f',2));
    
    //速度groupbox
    ui->avgSpeedValueLabel->setText(QString::number(ride.avgSpeed,'f',1));
    ui->maxSpeedValueLabel->setText(QString::number(ride.maxSpeed));

    //环境groupbox
    ui->avgTemperatureValueLabel->setText(QString::number(ride.avgTemperature,'f',1));
    ui->avgHumidityValueLabel->setText(QString::number(ride.avgHumidity,'f',1));

    //事件groupbox
    ui->eventCountValueLabel->setText(QString::number(ride.eventCount));
    
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
    SensorDataRepository repo(m_database->database());

    m_rawDataTotalRows = repo.countByRideId(rideId);
    int pageCount = (m_rawDataTotalRows + m_rawDataPageSize - 1) / m_rawDataPageSize;

    if (pageCount <= 0) {
        pageCount = 1;
    }

    if (m_rawDataPageIndex >= pageCount) {
        m_rawDataPageIndex = pageCount - 1;
    }

    int offset = m_rawDataPageIndex * m_rawDataPageSize;

    QSqlQuery query = repo.queryRawDataPageByRideId(
        rideId,
        m_rawDataPageSize,
        offset
    );

    if (repo.lastError().isEmpty() == false) {
        ui->statusbar->showMessage("原始数据查询失败: " + repo.lastError(), 5000);
        return;
    }

    m_rawDataModel->setQuery(std::move(query));
    updateRawDataPageControls();
}

int MainWindow::rawDataTotalCount(int rideId)
{
    QSqlQuery query(m_database->database());
    query.prepare(R"(
        select count(*)
        from sensor_data
        where ride_id=:ride_id
        )"
    );
    query.bindValue(":ride_id", rideId);

    if (!query.exec()) {
        ui->statusbar->showMessage("原始数据总数查询失败: " + query.lastError().text(), 5000);
        return 0;
    }

    if (!query.next()) {
        return 0;
    }

    return query.value(0).toInt();
}

void MainWindow::updateRawDataPageControls()
{
    int pageCount = (m_rawDataTotalRows + m_rawDataPageSize - 1) / m_rawDataPageSize;
    if (pageCount <= 0) {
        pageCount = 1;
    }

    ui->rawPageLabel->setText(QString("第 %1 / %2 页").arg(m_rawDataPageIndex + 1).arg(pageCount));
    ui->rawPrevButton->setEnabled(m_rawDataPageIndex > 0);
    ui->rawNextButton->setEnabled(m_rawDataPageIndex + 1 < pageCount);
}

void MainWindow::setIcon()
{
    this->setWindowIcon(QIcon(":/icon/icons/riding-line.png"));

    ui->distanceIconLabel->setScaledContents(true);
    ui->environmentIconLabel->setScaledContents(true);
    ui->speedIconLabel->setScaledContents(true);
    ui->spo2IconLabel->setScaledContents(true);
    ui->timeIconLabel->setScaledContents(true);
    ui->heartRateIconLabel->setScaledContents(true);

    ui->distanceIconLabel->setPixmap(QPixmap(":/icon/icons/distance.png"));
    ui->environmentIconLabel->setPixmap(QPixmap(":/icon/icons/environment.png"));
    ui->speedIconLabel->setPixmap(QPixmap(":/icon/icons/speed.png"));
    ui->spo2IconLabel->setPixmap(QPixmap(":/icon/icons/spo2.png"));
    ui->timeIconLabel->setPixmap(QPixmap(":/icon/icons/time.png"));
    ui->heartRateIconLabel->setPixmap(QPixmap(":/icon/icons/heartRate.png"));

}

void MainWindow::showRideCharts(int rideId)
{
    SensorDataRepository senRepo(m_database->database());
    QVector<SensorSample> samples=senRepo.findByRideId(rideId);

    QLineSeries *heartSeries=new QLineSeries();
    heartSeries->setName("心率");
    heartSeries->setColor(Qt::red);

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
    m_heartChartView->setCrosshairSeries(heartSeries,"心率","bpm",0);
    m_speedChartView->setChart(speedChart);
    m_speedChartView->setCrosshairSeries(speedSeries,"速度","km/h",2);
}

void MainWindow::showRideTrack(int rideId)
{
    SensorDataRepository repo(m_database->database());
    QVector<SensorSample> samples = repo.findByRideId(rideId);
    m_trackView->setTrackSample(samples);
}

void MainWindow::loadCurrentTab()
{
    if(m_currentRideId<=0) return;

    QWidget *currTab=ui->contentTabWidget->currentWidget();
    if(currTab==ui->dashboardTab){
        int row = ui->rideListWidget->currentRow();
        if (row >= 0 && row < m_rides.size()) {
            showRideDashboard(m_rides[row]);
        }
    }
    else if(currTab==ui->eventsTab){
        showRideEvents(m_currentRideId);
    }
    else if(currTab==ui->chartsTab){
        showRideCharts(m_currentRideId);
    }
    else if(currTab==ui->rawDataTab){
        showRideRawData(m_currentRideId);
    }
    else if(currTab==ui->trackTab){
        showRideTrack(m_currentRideId);
    }
}

void MainWindow::on_syncButton_clicked()
{
    qDebug()<<"-------------------------------";
    ui->syncButton->setEnabled(false);

    QSettings settings=SettingDialog::appSettings();

    OnenetRequestConfig config;
    config.productId=settings.value("onenet/productId","pmJ97H965j").toString();
    config.deviceName=settings.value("onenet/deviceName","dev1").toString();
    config.authorization=settings.value("onenet/authorization","version=2018-10-31&res=products%2FpmJ97H965j%2Fdevices%2Fdev1&et=1806050344&method=md5&sign=MBQgX0y0Megx1%2Bt8Zb3FkQ%3D%3D").toString();

    SyncRequest request;
    request.config = config;
    request.start_time = ui->startDateTimeEdit->dateTime().toMSecsSinceEpoch();
    request.end_time = ui->endDateTimeEdit->dateTime().toMSecsSinceEpoch();

    m_syncWorker->start(request);
}

void MainWindow::on_exportButton_clicked()
{
    if(m_currentRideId<=0){
        ui->statusbar->showMessage("请先选择一条骑行记录",5000);
        return;
    }

    SensorDataRepository senRepo(m_database->database());
    QVector<SensorSample> samples=senRepo.findByRideId(m_currentRideId);

    if(samples.isEmpty()){
        ui->statusbar->showMessage("当前骑行没有可导出的原始数据",5000);
        return;
    }

    QString filePath=QFileDialog::getSaveFileName(this,"选择导出csv的位置",QDir::homePath(),"CSV文件(*.csv)");

    if(filePath.isEmpty()){
        return;
    }

    if(!filePath.endsWith(".csv",Qt::CaseInsensitive)){
        filePath+=".csv";
    }

    QString errMsg;
    if(!CsvExporter::exportSamples(filePath,samples,&errMsg)){
        ui->statusbar->showMessage(errMsg,5000);
        return;
    }

    ui->statusbar->showMessage("CSV 导出成功: " + filePath, 5000);
}


void MainWindow::on_settingsButton_clicked()
{
    SettingDialog dialog(this);

    if(dialog.exec() != QDialog::Accepted){
        return;
    }

    QSettings settings=SettingDialog::appSettings();
    m_rawDataPageSize=settings.value("rawData/pageSize",20).toInt();

    m_rawDataPageIndex=0;
    m_rawDataTotalRows=0;

    loadCurrentTab();

    ui->statusbar->showMessage("设置已保存",3000);
}

