#include "OnenetClient.h"

#include <QUrlQuery>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
OnenetClient::OnenetClient(QObject *parent)
    :QObject(parent)
{
}

void OnenetClient::setConfig(const OnenetRequestConfig &config)
{
    m_config=config;
}

OnenetRequestConfig OnenetClient::config() const
{
    return m_config;
}

void OnenetClient::queryPropertyHistory(const QString &identifier, qint64 startTimeMs, qint64 endTimeMs, int offset, int limit)
{
    //组建url
    QUrl url("https://iot-api.heclouds.com/thingmodel/query-device-property-history");

    QUrlQuery query;
    query.addQueryItem("product_id",m_config.productId);
    query.addQueryItem("device_name",m_config.deviceName);
    query.addQueryItem("identifier",identifier);
    query.addQueryItem("start_time",QString::number(startTimeMs));
    query.addQueryItem("end_time",QString::number(endTimeMs));
    query.addQueryItem("sort","1");
    query.addQueryItem("offset",QString::number(offset));
    query.addQueryItem("limit",QString::number(limit));

    url.setQuery(query);

    //创建一个QNetworkRequest对象
    QNetworkRequest request=createRequest(url);
    //发送get请求
    QNetworkReply *reply=m_manager.get(request);

    //处理回复
    connect(reply,&QNetworkReply::finished,this,[=](){
        handlePropertyHistoryReply(reply,identifier,offset,limit);
    });
    
}

void OnenetClient::queryEventLog(const QString &identifier, qint64 startTimeMs, qint64 endTimeMs, int offset, int limit)
{
    //构建URL
    QUrl url("https://iot-api.heclouds.com/device/event-log");

    QUrlQuery query;
    query.addQueryItem("product_id",m_config.productId);
    query.addQueryItem("device_name",m_config.deviceName);
    query.addQueryItem("identifier",identifier);
    query.addQueryItem("start_time",QString::number(startTimeMs));
    query.addQueryItem("end_time",QString::number(endTimeMs));
    query.addQueryItem("offset",QString::number(offset));
    query.addQueryItem("limit",QString::number(limit));
    url.setQuery(query);

    //创建一个QNetworkRequest实例
    QNetworkRequest request=createRequest(url);

    //发送请求
    QNetworkReply * reply=m_manager.get(request);

    //处理回复
    connect(reply,&QNetworkReply::finished,this,[=](){
        handleEventLogReply(reply,offset,limit);
    });
}

QNetworkRequest OnenetClient::createRequest(const QUrl &url) const
{

    QNetworkRequest request(url);
    //设置认证凭证
    request.setRawHeader("authorization",m_config.authorization.toUtf8());
    //返回json格式
    request.setRawHeader("Accept","application/json");

    return request;
}

void OnenetClient::handlePropertyHistoryReply(QNetworkReply *reply, const QString &identifier, int offset, int limit)
{
    //
    reply->deleteLater();

    //检查是否有网络错误
    int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

    if (reply->error() != QNetworkReply::NoError) {
        emit requestFailed("queryPropertyHistory", httpStatus, reply->error(), reply->errorString());
        return;
    }


    //1.解析json
    QByteArray array=reply->readAll();
    if(array.isEmpty()) return;
    QJsonObject obj=arrayToJsonObject(array);

    //2.解析json对象获取时间戳和值
    QVector<OnenetPropertyPoint> point;

    //判断是否获取成功
    int code=obj.value("code").toInt();
    if(code!=0){
        qDebug()<<"[OnenetClient] GET failed! "<<obj.value("msg").toString();
        return;
    }

    //取data对象
    QJsonObject dataObj=obj.value("data").toObject();

    //取list
    QJsonArray listArray=dataObj.value("list").toArray();

    //遍历list取出数据
    for(const QJsonValue &itemVal : listArray){
        if(!itemVal.isObject()){
            return;
        }
        QJsonObject itemObj=itemVal.toObject();
        
        OnenetPropertyPoint data;
        data.timestampMs=itemObj.value("time").toVariant().toLongLong();
        data.value=itemObj.value("value").toString();
        point.append(data);
    
    }

    if(point.isEmpty()){
        qDebug()<<"[OnenetClient] didn't get data or null";
    }

    emit propertyHistoryReceived(identifier,point,offset,limit);
}

void OnenetClient::handleEventLogReply(QNetworkReply *reply, int offset, int limit)
{
    //
    reply->deleteLater();


    //检查是否有网络错误
    int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (reply->error() != QNetworkReply::NoError) {
        emit requestFailed("queryEventHistory", httpStatus, reply->error(), reply->errorString());
        return;
    }


    QByteArray array=reply->readAll();
    if(array.isEmpty()) return;

    QJsonObject rootObj=arrayToJsonObject(array);

    //是否成功请求到数据
    if(rootObj.value("code").toInt()!=0){
        qDebug()<<"[OnenetClient] GET failed! "<<rootObj.value("msg").toString();
        return;
    }

    //获取data
    QJsonObject dataObj=rootObj.value("data").toObject();

    //获取list
    QJsonArray list=dataObj.value("list").toArray();

    //
    QVector<OnenetEventItem> events;

    //遍历list
    for(auto itemVal : list){
        
        if(!itemVal.isObject()) return;
        QJsonObject itemObj=itemVal.toObject();
    
        OnenetEventItem item;
        item.timestampMs=itemObj.value("time").toVariant().toLongLong();
        item.eventType=itemObj.value("event_type").toInt();
        item.identifier=itemObj.value("identifier").toString();
        item.name=itemObj.value("name").toString();
        item.valueJson=itemObj.value("value").toString();
    
        events.append(item);
    }

    if(events.isEmpty()){
        qDebug()<<"[OnenetClient] didn't get data or null";
    }

    emit eventLogReceived(events,offset,limit);
}

QJsonObject OnenetClient::arrayToJsonObject(QByteArray &array)
{

    QJsonParseError err;
    QJsonDocument doc=QJsonDocument::fromJson(array,&err);
    if(!doc.isObject()){
        emit requestFailed("arrayToJsonObject",-2,0,"arrayToJsonObject failed");
        return QJsonObject();
    }

    return doc.object();
}
