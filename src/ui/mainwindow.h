#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "database/AppDatabase.h"
#include "service/SyncWorker.h"
#include "domain/Ride.h"
#include "ui/InteractiveChartView.h"
#include "ui/TrackView.h"


#include <vector>
#include <QSqlQueryModel>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>




QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    
    void setDateTimeEditRange();

    ~MainWindow() override;

    bool initDatabase();

    void loadRideList();

    void showRideDashboard(const Ride &ride);

    void showRideEvents(int rideId);

    void showRideRawData(int rideId);

    void showRideCharts(int rideId);

    void showRideTrack(int rideId);

    void loadCurrentTab();

    int rawDataTotalCount(int rideId);

    void updateRawDataPageControls();

    void setIcon();

private slots:
    void on_syncButton_clicked();

    void on_exportButton_clicked();

    void on_settingsButton_clicked();

private:
    Ui::MainWindow *ui;

    AppDatabase *m_database=nullptr;
    SyncWorker *m_syncWorker=nullptr;
    QVector<Ride> m_rides;

    QSqlQueryModel *m_rawDataModel=nullptr;
    
    InteractiveChartView *m_heartChartView=nullptr;
    InteractiveChartView *m_speedChartView=nullptr;

    TrackView *m_trackView=nullptr;

    int m_currentRideId=0;
    int m_rawDataPageIndex=0;
    int m_rawDataPageSize=20;
    int m_rawDataTotalRows=0;
};
#endif // MAINWINDOW_H
