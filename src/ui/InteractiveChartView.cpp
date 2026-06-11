#include "InteractiveChartView.h"

#include <QWheelEvent>
#include <QMouseEvent>
#include <QtCharts/QtCharts>

InteractiveChartView::InteractiveChartView(QWidget *parent)
    :QChartView(parent)
{
    // 1. 开启图表的抗锯齿渲染，使曲线和图形边缘更平滑
    setRenderHint(QPainter::Antialiasing);
    // 2. 设置橡皮筋（Rubber Band）交互模式为矩形选区，支持鼠标框选缩放
    setRubberBand(QChartView::RectangleRubberBand);
}

void InteractiveChartView::wheelEvent(QWheelEvent *ev)
{
    // 1. 检查是否存在图表对象，若不存在则交由基类处理
    if(!chart()){
        QChartView::wheelEvent(ev);
        return;
    }

    // 2. 根据滚轮滚动的方向进行缩放：向上滚动放大，向下滚动缩小
    if(ev->angleDelta().y()>0){
        chart()->zoom(1.15);                // 放大 1.15 倍
    }
    else{
        chart()->zoom(1.0/1.15);            // 缩小为原来的 1/1.15
    }

    // 3. 接受事件，阻止进一步传播
    ev->accept();
}

void InteractiveChartView::mousePressEvent(QMouseEvent *ev)
{
    // 1. 右键按下时：重置图表的缩放状态到初始视图
    if(ev->button()==Qt::RightButton){
        if (chart()){
            chart()->zoomReset();           // 重置缩放
        }
        ev->accept();
        return;
    }

    // 2. 中键按下时：进入平移模式，记录鼠标按下位置并改变光标样式
    if(ev->button()==Qt::MiddleButton){
        m_isPanning=true;                   // 标记为正在平移
        m_lastMousePos=ev->pos();           // 保存当前鼠标位置作为平移起点
        setCursor(Qt::ClosedHandCursor);    // 将光标改为抓取状态
        ev->accept();
        return;
    }

    // 3. 其他情况（如左键）：交由基类默认处理（例如橡皮筋框选）
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
    // 1. 中键释放且当前正处于平移模式时：结束平移，恢复光标样式
    if(m_isPanning && ev->button()==Qt::MiddleButton){
        m_isPanning=false;                  // 取消平移标记
        setCursor(Qt::ArrowCursor);         // 将光标恢复为默认箭头
        ev->accept();
        return;
    }

    // 2. 其他情况（如左键释放）：交由基类默认处理
    QChartView::mouseReleaseEvent(ev);
}
