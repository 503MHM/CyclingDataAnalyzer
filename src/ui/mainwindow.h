#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "database/AppDatabase.h"
#include "service/SyncWorker.h"
#include "domain/Ride.h"
#include <vector>


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

    void test();

private slots:
    void on_syncButton_clicked();

private:
    Ui::MainWindow *ui;

    AppDatabase *m_database=nullptr;
    SyncWorker *m_syncWorker=nullptr;
    QVector<Ride> m_rides;
};
#endif // MAINWINDOW_H
