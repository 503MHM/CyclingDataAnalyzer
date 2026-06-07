#include "SensorSampleBuilder.h"

#include <QDateTime>
SensorSampleBuilder::SensorSampleBuilder()
{
}

void SensorSampleBuilder::setDeviceAndRide(int deviceId, int rideId)
{
    m_deviceId=deviceId;
    m_rideId=rideId;
}

void SensorSampleBuilder::addPropertyPoints(const QString &identifier, const QVector<OnenetPropertyPoint> &points)
{
    for(const auto &point : points){
        qint64 bucketMs=bucketTimestamp(point.timestampMs);

        //还没有这一秒的数据
        if(!m_samples.contains(bucketMs)){
            SensorSample sample;
            sample.deviceId=m_deviceId;
            sample.rideId=m_rideId;
            sample.timestampMs=bucketMs;
            sample.timeText=QDateTime::fromMSecsSinceEpoch(bucketMs).toString(Qt::ISODate);
            sample.createdAt=QDateTime::currentDateTime().toString(Qt::ISODate);

            //插入一条新纪录
            m_samples.insert(bucketMs,sample);
        }

        //获取当前时间戳下的这条记录
        SensorSample &sample=m_samples[bucketMs];
        //设置记录的值
        setSampleValue(sample,identifier,point.value);

    }
}

QVector<SensorSample> SensorSampleBuilder::buildSamples() const
{
    QVector<SensorSample> samples=m_samples.values().toVector();

    //排序
    std::sort(samples.begin(),samples.end(),[](const SensorSample &left, const SensorSample &right){
        return left.timestampMs<right.timestampMs;
    });

    return samples;
}

//返回向下取整后的秒级时间戳
qint64 SensorSampleBuilder::bucketTimestamp(qint64 timestampMs) const
{
    return timestampMs/1000*1000;
}

void SensorSampleBuilder::setSampleValue(SensorSample &sample, const QString &identifier, const QString &value) const
{
    if (identifier == "heart_rate") {
        sample.heartRate = value.toInt();
    } else if (identifier == "spo2") {
        sample.spo2 = value.toInt();
    } else if (identifier == "speed") {
        sample.speed = value.toDouble();
    } else if (identifier == "temperature") {
        sample.temperature = value.toDouble();
    } else if (identifier == "humidity") {
        sample.humidity = value.toDouble();
    } else if (identifier == "latitude") {
        sample.latitude = value.toDouble();
    } else if (identifier == "longitude") {
        sample.longitude = value.toDouble();
    } else if (identifier == "trip_mileage") {
        sample.tripMileage = value.toDouble();
    } else if (identifier == "Total_Mileage") {
        sample.totalMileage = value.toDouble();
    }
}
