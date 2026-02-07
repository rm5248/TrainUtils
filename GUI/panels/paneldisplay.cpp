#include <QLayout>
#include <QPainterPath>
#include <QPainter>
#include <QHBoxLayout>
#include <QMouseEvent>

#include <log4cxx/logger.h>
#include <fmt/format.h>

#include "paneldisplay.h"
#include "panellayout.h"
#include "paneltoolswidget.h"

static log4cxx::LoggerPtr logger = log4cxx::Logger::getLogger("traingui.PanelDisplay");

PanelDisplay::PanelDisplay(QWidget *parent)
    : QWidget{parent}
{
//    PanelLayout* pl = new PanelLayout(this);
//    QHBoxLayout* pl = new QHBoxLayout(this);
    TurnoutDisplay* td = new TurnoutDisplay(this);
    td->setGeometry(50, 50, td->width(), td->height());
//    pl->addWidget(td);
//    this->setLayout(pl);
}

void PanelDisplay::paintEvent(QPaintEvent *event){
//    QWidget::paintEvent(event);
//    QPainterPath path;
//    path.moveTo(0, 0);
//    path.lineTo(200, 200);

//    path.moveTo(0, 200);
//    path.lineTo(200, 0);

//    QPainter painter(this);
//    painter.drawPath(path);
}

QSize PanelDisplay::sizeHint() const
{
    return QSize(600, 600);
}

void PanelDisplay::mousePressEvent(QMouseEvent* event){
    QWidget* widgetAtPos = childAt(event->pos());
    LOG4CXX_DEBUG_FMT(logger, "press button: {} pos: {},{} widget: {}",
                      (int)event->button(),
                      event->pos().x(),
                      event->pos().y(),
                      widgetAtPos ? "valid" : "invalid");

    if(!widgetAtPos){
        m_movingWidget = nullptr;
        m_tools->setCurrentSelectedWidget(nullptr);
        return;
    }
    m_tools->setCurrentSelectedWidget(widgetAtPos);

    if(event->button() == Qt::RightButton && m_allowMoving){
        m_movingWidget = widgetAtPos;
        m_movingWidgetStart = widgetAtPos->pos();
        m_mouseStart = event->pos();
    }else{
        m_movingWidget = nullptr;
    }
}

void PanelDisplay::mouseMoveEvent(QMouseEvent *event){
    if(m_movingWidget == nullptr){
        return;
    }

    int newX = m_movingWidgetStart.x();
    int newY = m_movingWidgetStart.y();
    int diffX = std::abs(m_mouseStart.x() - event->pos().x());
    int diffY = std::abs(m_mouseStart.y() - event->pos().y());
    if(event->pos().x() < m_mouseStart.x()){
        newX -= diffX;
    }else{
        newX += diffX;
    }
    if(event->pos().y() < m_mouseStart.y()){
        newY -= diffY;
    }else{
        newY += diffY;
    }

    m_movingWidget->setGeometry(newX, newY, m_movingWidget->width(), m_movingWidget->height());
}

void PanelDisplay::setPanelToolsWidget(PanelToolsWidget* widget){
    m_tools = widget;
}

void PanelDisplay::allowMovingChanged(bool allow_moving){
    m_allowMoving = allow_moving;

    if(!m_allowMoving){
        m_movingWidget = nullptr;
    }
}
