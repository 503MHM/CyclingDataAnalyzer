#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "database/AppDatabase.h"
#include "service/SyncWorker.h"
#include "domain/Ride.h"
#include "ui/InteractiveChartView.h"

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

    void test();

private slots:
    void on_syncButton_clicked();

private:
    Ui::MainWindow *ui;

    AppDatabase *m_database=nullptr;
    SyncWorker *m_syncWorker=nullptr;
    QVector<Ride> m_rides;

    QSqlQueryModel *m_rawDataModel=nullptr;
    
    InteractiveChartView *m_heartChartView=nullptr;
    InteractiveChartView *m_speedChartView=nullptr;
};
#endif // MAINWINDOW_H
