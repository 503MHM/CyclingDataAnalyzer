#pragma once

#include "domain/Device.h"

#include <QSqlDatabase>

class DeviceRepository{

public:
    DeviceRepository(const QSqlDatabase &database);

    std::optional<Device> findProductAndName(const QString &productId, const QString &deviceName);

    int upsertDevice(const QString &productId, const QString &deviceName,const QString &displayName=QString());

    QString lastError();

private:
    QSqlDatabase m_database;
    QString m_lastError;

};