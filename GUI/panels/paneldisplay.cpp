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

    if(m_drawConnectionPoints){
        // Create a list of all of the connection points, and let's go draw them
        m_connectionPoints.clear();
        for(TurnoutDisplay* disp : m_turnouts){
            for(QPoint& p : disp->connectionPoints()){
                QPoint newPoint(disp->pos() + p);
                newPoint.setX(newPoint.x() - 5);
                newPoint.setY(newPoint.y() - 5);
                m_connectionPoints.push_back(newPoint);
            }
        }


        for(QPoint& center : m_connectionPoints){
            painter.save();
            painter.translate(center);

            // a connection point is just a white semi-transparent box
            // with a black border
            QPoint topLeft(0, 0);
            QPoint topRight(10, 0);
            QPoint bottomRight(10, 10);
            QPoint bottomLeft(0, 10);
            QRect box(topLeft, bottomRight);

            // draw the semi-transparent part first
            QBrush semiTransparent(QColor(255, 255, 255, 128));
            painter.fillRect(box, semiTransparent);

            QPainterPath path;
            path.moveTo(topLeft);
            path.lineTo(topRight);
            path.lineTo(bottomRight);
            path.lineTo(bottomLeft);
            path.lineTo(topLeft);

            QPen pen;
            pen.setWidth(2);
            pen.setBrush(Qt::black);
            painter.setPen(pen);

            painter.drawPath(path);
            painter.restore();
        }
    }
}

void PanelDisplay::addTurnout(std::shared_ptr<Turnout> turnout){
    TurnoutDisplay* td = new TurnoutDisplay(this);
    td->setTurnout(turnout);
    td->setGeometry(50, 50, td->width(), td->height());
    td->setVisible(true);
    m_turnouts.push_back(td);

    connect(td, &TurnoutDisplay::connectionPointsUpdated,
            this, &PanelDisplay::connectionPointsUpdated);
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

    // First let's check to see if we are selecting a connection point
    if(m_drawConnectionPoints && event->button() == Qt::LeftButton){
        QPoint mousePos = event->pos();
        for(QPoint& point : m_connectionPoints){
            QPoint distance = point - mousePos;
            if(distance.manhattanLength() < 5){
                // We are in a connection point!
                m_connectingState = ConnectingState::Connecting;
                m_startConnectingPoint = point;
            }
        }
        return;
    }

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
    connect(widget, &PanelToolsWidget::drawConnectionPointsChanged,
            this, &PanelDisplay::drawConnectionPointsChanged);
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

    connect(td, &TurnoutDisplay::connectionPointsUpdated,
            this, &PanelDisplay::connectionPointsUpdated);
}

void PanelDisplay::connectionPointsUpdated(){
    update(this->rect());
}

void PanelDisplay::drawConnectionPointsChanged(bool connection_points){
    m_drawConnectionPoints = connection_points;
    update(this->rect());
}
