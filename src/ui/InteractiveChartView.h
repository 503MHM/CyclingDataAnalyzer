#pragma once

#include <QChartView>


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

};
