#include "service/AnalysisService.h"
#include "domain/SensorSample.h"

#include <QVector>

AnalysisService::RideStats AnalysisService::calculateRideStats(const QVector<SensorSample> &samples, const QVector<RideEvent> &events)
{

    int heart_rate_sum=0;
    int heart_cnt=0;
    int max_heart_rate=0;
    double speed_sum=0;
    double max_speed=0.0;
    int spo2_sum=0;
    int spo2_cnt=0;
    int min_spo2=0;
    double temperature_sum=0;
    double humidity_sum=0; 


    for(const SensorSample &sample : samples){
        //心率
        if(sample.heartRate!=0){
            heart_cnt++;
            heart_rate_sum+=sample.heartRate;

            if(sample.heartRate>max_heart_rate) max_heart_rate=sample.heartRate;
        }

        //血氧
        if(sample.spo2!=0){
            spo2_cnt++;
            spo2_sum+=sample.spo2;

            if(min_spo2 == 0 || sample.spo2<min_spo2) min_spo2=sample.spo2;
        }

        speed_sum+=sample.speed;
        if(sample.speed>max_speed) max_speed=sample.speed;
        temperature_sum+=sample.temperature;
        humidity_sum+=sample.humidity;

    }
    RideStats stats;
    stats.avgHeartRate = heart_cnt > 0 ? (heart_rate_sum*1.0)/(heart_cnt*1.0) : 0.0;
    stats.avgSpo2 = spo2_cnt > 0 ? (spo2_sum*1.0)/(spo2_cnt*1.0) : 0.0;
    stats.avgHumidity = !samples.isEmpty() ? humidity_sum/(samples.count()*1.0) : 0.0;
    stats.avgSpeed = !samples.isEmpty() ? speed_sum/(samples.count()*1.0) : 0.0;
    stats.avgTemperature = !samples.isEmpty() ? temperature_sum/(samples.count()*1.0) : 0.0;
    stats.eventCount=events.count();
    stats.maxHeartRate=max_heart_rate;
    stats.maxSpeed=max_speed;
    stats.minSpo2=min_spo2;

    return stats;
}
