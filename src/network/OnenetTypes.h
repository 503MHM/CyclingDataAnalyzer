#pragma once

#include <QString>


struct OnenetPropertyPoint
{
    qint64 timestampMs=0;
    QString value;
};


struct OnenetEventItem
{
    qint64 timestampMs=0;
    QString identifier;
    QString name;
    int eventType=0;
    QString valueJson;

};


struct OnenetRequestConfig
{
    QString productId;
    QString deviceName;
    QString authorization;
};
