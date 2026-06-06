#include "DeviceRepository.h"

#include <qsqlquery.h>
#include <QDateTime>
#include <QSqlError>

DeviceRepository::DeviceRepository(const QSqlDatabase &database)
{
    m_database=database;
}

std::optional<Device> DeviceRepository::findProductAndName(const QString &productId, const QString &deviceName)
{
    QSqlQuery query(m_database);
    query.prepare(R"(
        select *
        from devices
        where product_id= :product_id and device_name= :device_name
    )");
    query.bindValue(":product_id",productId);
    query.bindValue(":device_name",deviceName);
    
    Device dev;

    //判断是否有设备
    if(query.exec()){

        //找到了设备,返回
        if(query.next()){  
            dev.id=query.value(0).toInt();
            dev.productId=query.value(1).toString();
            dev.deviceName=query.value(2).toString();
            dev.displayName=query.value(3).toString();
            dev.remark=query.value(4).toString();
            dev.createdAt=query.value(5).toString();
            dev.lastSyncTime=query.value(6).toString();

            return dev;
        }

        else{
            return std::nullopt;
        }
    }
    else{
        m_lastError=query.lastError().text();
        return std::nullopt;
    }


    return std::optional<Device>();
}

int DeviceRepository::upsertDevice(const QString &productId, const QString &deviceName, const QString &displayName)
{

    std::optional<Device> dev=findProductAndName(productId,deviceName);
    QSqlQuery query(m_database);
    
    if(dev){
        return dev.value().id;
    }
    
    query.prepare(R"(
        INSERT INTO devices (
            product_id,
            device_name,
            display_name,
            remark,
            created_at,
            last_sync_time
        ) VALUES (
            :product_id,
            :device_name,
            :display_name,
            :remark,
            :created_at,
            :last_sync_time
        )
    )");

    const QString now = QDateTime::currentDateTime().toString(Qt::ISODate);

    query.bindValue(":product_id", productId);
    query.bindValue(":device_name", deviceName);
    query.bindValue(":display_name", displayName);
    query.bindValue(":remark", QString());
    query.bindValue(":created_at", now);
    query.bindValue(":last_sync_time", QString());

    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return 0;
    }

    return query.lastInsertId().toInt();
}

QString DeviceRepository::lastError()
{
    return m_lastError;
}
