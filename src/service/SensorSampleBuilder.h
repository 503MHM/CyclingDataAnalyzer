#pragma once

#include "domain/SensorSample.h"
#include "network/OnenetTypes.h"

#include <QVector>
#include <QHash>


class SensorSampleBuilder
{
    
public:
    SensorSampleBuilder();

    void setDeviceAndRide(int deviceId,int rideId);

    void addPropertyPoints(const QString &identifier,const QVector<OnenetPropertyPoint> &points);

    QVector<SensorSample> buildSamples() const;

private:
    qint64 bucketTimestamp(qint64 timestampMs) const;
    void setSampleValue(SensorSample &sample,const QString &identifier,const QString &value) const;

private:
    int m_deviceId=0;
    int m_rideId=0;
    QHash<qint64,SensorSample> m_samples;

};
