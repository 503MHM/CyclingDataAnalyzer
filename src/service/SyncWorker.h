#pragma once

#include <QObject>
#include <QSqlDatabase>
#include <QHash>
#include <QVector>

#include "network/OnenetClient.h"
#include "network/OnenetTypes.h"



struct SyncRequest
{
    OnenetRequestConfig config;
    qint64 start_time=0;
    qint64 end_time=0;
};


class SyncWorker:public QObject
{
    Q_OBJECT
public:
    SyncWorker(const QSqlDatabase &database, QObject *parent=nullptr);

    void start(SyncRequest request);

signals:
    void progressChanged(const QString &typeMessage,int current,int total);

    void finished(int rideId);

    void failed(const QString &message);


private:
    void tryFinish();

    qint64 isoDateStringToMs(const QString &timeText);

    QHash<QString,QVector<OnenetPropertyPoint>> filterPropertyPointsByTime(
        const QHash<QString, QVector<OnenetPropertyPoint>> &allPoints,
        qint64 startMs,
        qint64 endMs
    );

    QVector<OnenetEventItem> filterEventByTime(
        const QVector<OnenetEventItem> &events,
        qint64 startMs,
        qint64 endMs
    );


private:
    QSqlDatabase m_database;
    OnenetClient *m_client=nullptr;

    SyncRequest m_request;
    int m_deviceId=0;

    QHash<QString,QVector<OnenetPropertyPoint>> m_propertyPoints;
    QVector<OnenetEventItem> m_eventItems;
    
    int m_receivedPropertyCount = 0;
    int m_expectedPropertyCount = 0;
    int m_receivedEventCount = 0;
    int m_expectedEventCount = 0;
    bool m_finished=false;
};
