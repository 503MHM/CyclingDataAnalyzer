#pragma once

#include <QObject>
#include "network/OnenetTypes.h"

#include <QNetworkAccessManager>
#include <QDateTime>
#include <QUrl>

class OnenetClient : public QObject
{
    Q_OBJECT

public:
    OnenetClient(QObject *parent=nullptr);

    void setConfig(const OnenetRequestConfig &config);
    OnenetRequestConfig config() const;

    void queryPropertyHistory(const QString &identifier,qint64 startTimeMs,qint64 endTimeMs,int offset=0,int limit=1000);
    void queryEventLog(const QString &identifier,qint64 startTimeMs,qint64 endTimeMs,int offset=0,int limit=100);

signals:
    void propertyHistoryReceived(const QString &identifier,const QVector<OnenetPropertyPoint> &point,int offset,int limit);

    void eventLogReceived(const QVector<OnenetEventItem> &events,int offset,int limit);

    void requestFailed(const QString &operation,int httpStatus,int errorCode,const QString &message);

private:
    QNetworkRequest createRequest(const QUrl &url) const;

    void handlePropertyHistoryReply(QNetworkReply *reply,const QString &identifier,int offset,int limit);

    void handleEventLogReply(QNetworkReply *reply,int offset,int limit);

    QJsonObject arrayToJsonObject(QByteArray &array);

private:
    QNetworkAccessManager m_manager;
    OnenetRequestConfig m_config;
};