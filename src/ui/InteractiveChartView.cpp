#include "InteractiveChartView.h"

#include <QWheelEvent>
#include <QMouseEvent>
#include <QtCharts/QtCharts>

InteractiveChartView::InteractiveChartView(QWidget *parent)
    :QChartView(parent)
{
    setRenderHint(QPainter::Antialiasing);              //开启图表的抗锯齿渲染
    setRubberBand(QChartView::RectangleRubberBand);     //设置图表的橡皮筋（Rubber Band）交互模式，具体为矩形选区模式。
}

void InteractiveChartView::wheelEvent(QWheelEvent *ev)
{
    if(!chart()){
        QChartView::wheelEvent(ev);
        return;
    }

    if(ev->angleDelta().y()>0){
        chart()->zoom(1.15);
    }
    else{
        chart()->zoom(1.0/1.15);
    }

    ev->accept();
}

void InteractiveChartView::mousePressEvent(QMouseEvent *ev)
{
    if(ev->button()==Qt::RightButton){
        //右键重置图表缩放
        if (chart()){
            chart()->zoomReset();
        }
        ev->accept();
        return;
    }

    if(ev->button()==Qt::MiddleButton){
        m_isPanning=true;
        m_lastMousePos=ev->pos();
        setCursor(Qt::ClosedHandCursor);
        ev->accept();
        return;
    }

    QChartView::mousePressEvent(ev);

}

void InteractiveChartView::mouseMoveEvent(QMouseEvent *ev)
{
    if(m_isPanning && chart()){
        // 2. 计算鼠标移动的增量
        QPoint delta=ev->pos()-m_lastMousePos;
        // 3. 调用图表的scroll方法平移图表
        chart()->scroll(-delta.x(),delta.y());
        // 4. 更新上次鼠标位置为当前位置，用于下次移动
        m_lastMousePos=ev->pos();

        ev->accept();
        return;
    }

    QChartView::mouseMoveEvent(ev);

}

void InteractiveChartView::mouseReleaseEvent(QMouseEvent *ev)
{
    if(m_isPanning && ev->button()==Qt::MiddleButton){
        m_isPanning=false;
        setCursor(Qt::ArrowCursor);
        return;
    }

    QChartView::mouseReleaseEvent(ev);
}
