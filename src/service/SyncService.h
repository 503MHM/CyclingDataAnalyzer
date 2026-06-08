#pragma once

#include <QSqlDatabase>
#include <QString>

#include "network/OnenetTypes.h"

class SyncService
{

private:
    QSqlDatabase m_database;
    QString m_lastError;

public:
    SyncService(const QSqlDatabase &database);

    bool syncPropertiesToRide(int deviceId,int rideId,const QHash<QString,QVector<OnenetPropertyPoint> > &propertyPoints);

    bool syncEventsToRide(int deviceId,int rideId,const QVector<OnenetEventItem> &items);

    QString lastError() const;

};
