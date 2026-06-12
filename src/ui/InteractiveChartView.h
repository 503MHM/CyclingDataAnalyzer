#pragma once

#include <QtCharts/QChartView>
#include <QPoint>
#include <QLineSeries>
#include <QLabel>
#include <QEvent>
#include <QPaintEvent>

class InteractiveChartView :public QChartView
{
    Q_OBJECT
public:
    InteractiveChartView(QWidget *parent=nullptr);

    void setCrosshairSeries(QLineSeries *series,const QString &label,const QString &unit,int precision);

protected:
    void wheelEvent(QWheelEvent *ev)override;
    void mousePressEvent(QMouseEvent *ev)override;
    void mouseMoveEvent(QMouseEvent *ev)override;
    void mouseReleaseEvent(QMouseEvent *ev)override;
    void leaveEvent(QEvent *ev)override;
    void paintEvent(QPaintEvent *ev)override;

private:
    void updateCrosshair(const QPoint &mousePos);
    //获取真正绘图区像素范围
    QRect currentPlotRect() const;

private:
    bool m_isPanning=false;
    QPoint m_lastMousePos;

    QLineSeries *m_crosshairSeries=nullptr;         //表示当前这张图要参考哪条线来找最近点
    QLabel *m_tooltipLabel=nullptr;                 //悬浮提示框
    bool m_crosshairVisible=false;                  //表示虚线和圆点当前要不要显示
    int m_crosshairX=0;                             //竖向虚线的 x 坐标，单位是界面像素（控件点的真实位置）
    QPoint m_markerPos;                             //圆点的位置，单位是界面像素（控件点的真实位置）
    QPointF m_nearestPoint;                         //最近的数据点，用来生成 tooltip 文本

    //用来显示 tooltip。
    QString m_crosshairLabel;
    QString m_crosshairUnit;
    int m_crosshairPrecision=1;


};
