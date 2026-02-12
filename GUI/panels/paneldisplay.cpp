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
    QWidget::paintEvent(event);
    QPainter painter(this);
//    painter.drawPath(path);

    if(m_selectedWidget){
        // Draw a box around the widget
        QPoint topLeft = m_selectedWidget->pos();
        QPoint topRight(topLeft.x() + m_selectedWidget->size().width(), topLeft.y());
        QPoint bottomRight(topLeft.x() + m_selectedWidget->size().width(), topLeft.y() + m_selectedWidget->size().height());
        QPoint bottomLeft(topLeft.x(), topLeft.y() + m_selectedWidget->size().height());
        QPainterPath path;
        path.moveTo(topLeft);
        path.lineTo(topRight);
        path.lineTo(bottomRight);
        path.lineTo(bottomLeft);
        path.lineTo(topLeft);

        QPen pen;
        pen.setWidth(3);
        pen.setBrush(Qt::green);
        pen.setStyle(Qt::DashDotLine);
        painter.setPen(pen);

        painter.drawPath(path);
    }
}

void PanelDisplay::addTurnout(std::shared_ptr<Turnout> turnout){
    TurnoutDisplay* td = new TurnoutDisplay(this);
    td->setTurnout(turnout);
    td->setGeometry(50, 50, td->width(), td->height());
    td->setVisible(true);
    m_turnouts.push_back(td);
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
        m_selectedWidget = nullptr;
        m_tools->setCurrentSelectedWidget(nullptr);
        update(this->rect());
        return;
    }
    m_tools->setCurrentSelectedWidget(widgetAtPos);

    if(event->button() == Qt::RightButton && m_allowMoving){
        m_selectedWidget = widgetAtPos;
        m_movingWidgetStart = widgetAtPos->pos();
        m_mouseStart = event->pos();
    }else{
        m_selectedWidget = nullptr;
    }

    update(this->rect());
}

void PanelDisplay::mouseMoveEvent(QMouseEvent *event){
    if(m_selectedWidget == nullptr){
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

    m_selectedWidget->setGeometry(newX, newY, m_selectedWidget->width(), m_selectedWidget->height());
    update(this->rect());
}

void PanelDisplay::setPanelToolsWidget(PanelToolsWidget* widget){
    m_tools = widget;

    connect(widget, &PanelToolsWidget::addDCCTurnout,
            this, &PanelDisplay::addBlankTurnout);
}

void PanelDisplay::allowMovingChanged(bool allow_moving){
    m_allowMoving = allow_moving;

    if(!m_allowMoving){
        m_selectedWidget = nullptr;
    }

    for(TurnoutDisplay* td : m_turnouts){
        td->configureInteraction(!allow_moving);
    }
}

void PanelDisplay::addBlankTurnout(){
    LOG4CXX_DEBUG(logger, "Adding blank turnout");
    TurnoutDisplay* td = new TurnoutDisplay(this);
    td->setGeometry(10, 10, td->width(), td->height());
    td->setVisible(true);
    m_turnouts.push_back(td);
    update(this->rect());
    td->update();
}
