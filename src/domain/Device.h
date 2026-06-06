#pragma once

#include <QString>

struct Device{
    int id=0;
    QString productId;
    QString deviceName;
    QString displayName;
    QString remark;
    QString createdAt;
    QString lastSyncTime;
};