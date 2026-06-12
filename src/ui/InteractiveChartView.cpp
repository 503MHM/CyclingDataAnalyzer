#include "InteractiveChartView.h"

#include <QWheelEvent>
#include <QMouseEvent>
#include <QtCharts/QtCharts>
#include <QRectF>
#include <QPainter>

InteractiveChartView::InteractiveChartView(QWidget *parent)
    :QChartView(parent)
{
    // 1. 开启图表的抗锯齿渲染，使曲线和图形边缘更平滑
    setRenderHint(QPainter::Antialiasing);
    // 2. 设置橡皮筋（Rubber Band）交互模式为矩形选区，支持鼠标框选缩放
    setRubberBand(QChartView::RectangleRubberBand);

    setMouseTracking(true);
    viewport()->setMouseTracking(true);

    m_tooltipLabel=new QLabel(this);
    m_tooltipLabel->setStyleSheet(
        "QLabel {"
        "background: white;"
        "border: 1px solid #d0d7de;"
        "border-radius: 6px;"
        "padding: 6px 8px;"
        "}"
    );
    m_tooltipLabel->hide();

}

void InteractiveChartView::setCrosshairSeries(QLineSeries *series, const QString &label, const QString &unit, int precision)
{
    m_crosshairSeries=series;
    m_crosshairLabel=label;
    m_crosshairUnit=unit;
    m_crosshairPrecision=precision;
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

    updateCrosshair(ev->pos());
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

void InteractiveChartView::leaveEvent(QEvent *ev)
{
    m_crosshairVisible=false;

    if(m_tooltipLabel){
        m_tooltipLabel->hide();
    }

    viewport()->update();

    QChartView::leaveEvent(ev);
}

void InteractiveChartView::paintEvent(QPaintEvent *ev)
{
    QChartView::paintEvent(ev);

    if(!m_crosshairVisible || !chart()){
        return;
    }

    QRect plotRect=currentPlotRect();

    //准备绘图工具
    QPainter painter(viewport());
    painter.setRenderHint(QPainter::Antialiasing);

    //画竖向虚线
    QPen linePen(QColor("#C0C0C0"));
    linePen.setStyle(Qt::DashLine);
    linePen.setWidth(1);
    painter.setPen(linePen);

    painter.drawLine(QPoint(m_crosshairX,plotRect.top()),QPoint(m_crosshairX,plotRect.bottom()));

    //画圆点标记
    painter.setPen(QPen(QColor("#1a73e8"), 2));
    painter.setBrush(Qt::white);
    painter.drawEllipse(m_markerPos, 4, 4);
}

void InteractiveChartView::updateCrosshair(const QPoint &mousePos)
{
    //检查是否有效
    if(!chart() || !m_crosshairSeries || m_crosshairSeries->points().isEmpty()){
        return;
    }

    //判断鼠标是否在绘画区
    QRect plotRect=currentPlotRect();
    if(!plotRect.contains(mousePos)){
        m_crosshairVisible=false;
        m_tooltipLabel->hide();
        viewport()->update();
        return;
    }

    //将鼠标位置转换为数据坐标
    QPoint chartPos=mapFromScene(mapToScene(mousePos));                     //chart内部的逻辑坐标
    QPointF value=chart()->mapToValue(chartPos,m_crosshairSeries);          //数据坐标(不一定正好落在某个数据点上，可能位于两个数据点之间)

    //寻找最近的数据点
    const auto points=m_crosshairSeries->points();

    QPointF nearest=points.first();                                         //最近数据点
    double shortestDistance=qAbs(nearest.x()-value.x());                    //最短距离

    for(const QPointF &point : points){
        double distance=qAbs(point.x()-value.x());

        if(distance<shortestDistance){
            nearest=point;
            shortestDistance=distance;
        }
    }

    // 将最近点转换为控件坐标
    QPointF chartNearestPos=chart()->mapToPosition(nearest);
    QPoint markerViewPos=mapFromScene(chart()->mapToScene(chartNearestPos));

    m_nearestPoint=nearest;
    m_markerPos=markerViewPos;
    m_crosshairX=markerViewPos.x();
    m_crosshairVisible=true;

    //生成提示框文本
    QString time=QDateTime::fromMSecsSinceEpoch(static_cast<qint64>(nearest.x())).toString("yyyy-MM-dd HH:mm:ss");
    m_tooltipLabel->setText(
        QString("%1\n%2: %3 %4").arg(time).arg(m_crosshairLabel).arg(nearest.y(),0,'f',m_crosshairPrecision).arg(m_crosshairUnit)
    );

    //调整提示框位置
    m_tooltipLabel->adjustSize();               //自动调整大小
    int tooltipX=markerViewPos.x()+12;
    int tooltipY=markerViewPos.y()-12;

    if(tooltipX+m_tooltipLabel->width() > plotRect.right()){
        tooltipX=markerViewPos.x()-m_tooltipLabel->width()-12;
    }

    m_tooltipLabel->move(tooltipX,tooltipY);
    m_tooltipLabel->show();


    viewport()->update();
}


QRect InteractiveChartView::currentPlotRect() const
{
    if(!chart()) return QRect();

    QRectF plotArea=chart()->plotArea();

    QPoint topLeft=mapFromScene(chart()->mapToScene(plotArea.topLeft()));
    QPoint bottomRigh=mapFromScene(chart()->mapToScene(plotArea.bottomRight()));


    return QRect(topLeft,bottomRigh);
}
