#pragma once

#include <QtCharts/QChartView>
#include <QPoint>
#include <QLineSeries>
#include <QLabel>

class InteractiveChartView :public QChartView
{
    Q_OBJECT
public:
    InteractiveChartView(QWidget *parent=nullptr);

protected:
    void wheelEvent(QWheelEvent *ev)override;
    void mousePressEvent(QMouseEvent *ev)override;
    void mouseMoveEvent(QMouseEvent *ev)override;
    void mouseReleaseEvent(QMouseEvent *ev)override;

private:
    bool m_isPanning=false;
    QPoint m_lastMousePos;

    QLineSeries *m_crosshairSeries=nullptr;         //表示当前这张图要参考哪条线来找最近点
    QLabel *m_tooltipLabel=nullptr;                 //悬浮提示框
    bool m_crosshairVisible=false;                  //表示虚线和圆点当前要不要显示
    int m_crosshairX=0;                             //竖向虚线的 x 坐标




};
