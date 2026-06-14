#include "TrackView.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QVBoxLayout>

#include "Wgs84ToGcj02.h"

TrackView::TrackView(QWidget *parent)
    :QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_webView = new QWebEngineView(this);
    layout->addWidget(m_webView);

    //页面加载完成
    connect(m_webView,&QWebEngineView::loadFinished,this,[=](bool ok){
        if(!ok) return;
        m_pageLoad=true;
        if(!m_pendingSamples.isEmpty()){
            QString json = buildTrackJson(m_pendingSamples);
            m_lastTrackJson = json;
            QString js   = QString("setTrack(%1);").arg(json);
            m_webView->page()->runJavaScript(js);
        }
    });

    //加载本地html
    m_webView->load(QUrl("qrc:/html/track_map.html"));
}

void TrackView::setTrackSample(const QVector<SensorSample> samples)
{
    m_pendingSamples=samples;

    if(!m_pageLoad) return;

    if(samples.isEmpty()){
        clearTrack();
        return;
    }

    //加载完成发送数据
    QString json = buildTrackJson(m_pendingSamples);
    if (json == m_lastTrackJson) {
        return;
    }
    m_lastTrackJson = json;

    QString js   = QString("setTrack(%1);").arg(json);
    m_webView->page()->runJavaScript(js);
}

void TrackView::clearTrack()
{
    m_pendingSamples.clear();
    m_lastTrackJson.clear();
    if (!m_pageLoad) return;
    m_webView->page()->runJavaScript("clearTrack();");
}

QString TrackView::buildTrackJson(const QVector<SensorSample> &samples) const
{
    QJsonArray arr;

    for(auto &sample : samples){
        if(sample.latitude==0 && sample.longitude==0) continue;

        Coord coord=wgs84ToGcj02(sample.latitude,sample.longitude);


        QJsonObject obj;
        obj["lat"]=coord.lat;
        obj["lng"]=coord.lng;
        obj["time"]=sample.timeText;
        obj["speed"]=sample.speed;

        arr.append(obj);
    }

    QJsonDocument doc(arr);
    return QString::fromUtf8(doc.toJson(QJsonDocument::Compact));
}
