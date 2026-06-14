#pragma once

#include <QWidget>
#include <QWebEngineView>

#include "domain/SensorSample.h"

class TrackView : public QWidget
{

public:
    TrackView(QWidget *parent=nullptr);
    
    void setTrackSample(const QVector<SensorSample> samples);
    void clearTrack();

private:
    QWebEngineView *m_webView=nullptr;
    bool m_pageLoad=false;
    QVector<SensorSample> m_pendingSamples;
    QString m_lastTrackJson;


    QString buildTrackJson(const QVector<SensorSample> &samples) const;

};
