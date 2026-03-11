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
    m_name = "Panel";
}

void PanelDisplay::paintEvent(QPaintEvent *event){
    QWidget::paintEvent(event);
    QPainter painter(this);

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
        // Rebuild the cached list of all connection points
        m_connectionPoints.clear();
        for(TurnoutDisplay* disp : m_turnouts){
            int idx = 0;
            for(const QPoint& p : disp->connectionPoints()){
                QPoint center = disp->pos() + p;
                m_connectionPoints.push_back({disp, idx, center});
                idx++;
            }
        }
        for(const SegmentConnection& sc : m_segments){
            int idx = 0;
            for(const QPoint& p : sc.segment->connectionPoints()){
                m_connectionPoints.push_back({sc.segment, idx++, sc.segment->pos() + p});
            }
        }

        for(const CachedConnectionPoint& cp : m_connectionPoints){
            painter.save();
            QPoint topLeft(cp.center.x() - 5, cp.center.y() - 5);
            painter.translate(topLeft);

            // a connection point is just a white semi-transparent box
            // with a black border
            QRect box(0, 0, 10, 10);

            QBrush semiTransparent(QColor(255, 255, 255, 128));
            painter.fillRect(box, semiTransparent);

            QPainterPath path;
            path.addRect(box);

            QPen pen;
            pen.setWidth(2);
            pen.setBrush(Qt::black);
            painter.setPen(pen);

            painter.drawPath(path);
            painter.restore();
        }

        // Draw the in-progress connection line while dragging
        if(m_connectingState == ConnectingState::Connecting){
            QWidget* startWidget = dynamic_cast<QWidget*>(m_startEndpoint.connectable);
            QPoint startCenter = startWidget->pos()
                                 + m_startEndpoint.connectable->connectionPoints()[m_startEndpoint.index];
            QPen linePen;
            linePen.setWidth(2);
            linePen.setBrush(Qt::darkGray);
            linePen.setStyle(Qt::DashLine);
            painter.setPen(linePen);
            painter.drawLine(startCenter, m_currentMousePos);
        }
    }
}

void PanelDisplay::addTurnout(std::shared_ptr<Turnout> turnout){
    TurnoutDisplay* td = new TurnoutDisplay(this);
    td->setTurnout(turnout);
    td->setGeometry(50, 50, td->width(), td->height());
    td->setVisible(true);
    td->configureInteraction(m_allowMoving);
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
        for(const CachedConnectionPoint& cp : m_connectionPoints){
            QRect box(cp.center.x() - 5, cp.center.y() - 5, 10, 10);
            if(box.contains(mousePos)){
                m_connectingState = ConnectingState::Connecting;
                m_startEndpoint = {cp.connectable, cp.index};
                m_currentMousePos = mousePos;
                return;
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
    if(m_connectingState == ConnectingState::Connecting){
        m_currentMousePos = event->pos();
        update(this->rect());
        return;
    }

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

    // Update any segments whose endpoints are attached to the moved widget
    Connectable* movedConnectable = dynamic_cast<Connectable*>(m_selectedWidget);
    if(movedConnectable){
        for(SegmentConnection& sc : m_segments){
            if(sc.a.connectable == movedConnectable || sc.b.connectable == movedConnectable){
                sc.segment->setEndpoints(endpointPos(sc.a), endpointPos(sc.b));
            }
        }
    }

    update(this->rect());
}

void PanelDisplay::mouseReleaseEvent(QMouseEvent* event){
    if(m_connectingState != ConnectingState::Connecting || event->button() != Qt::LeftButton){
        return;
    }

    m_connectingState = ConnectingState::NotConnecting;

    QPoint mousePos = event->pos();
    for(const CachedConnectionPoint& cp : m_connectionPoints){
        // Don't connect a point to itself
        if(cp.connectable == m_startEndpoint.connectable && cp.index == m_startEndpoint.index){
            continue;
        }
        QRect box(cp.center.x() - 5, cp.center.y() - 5, 10, 10);
        if(box.contains(mousePos)){
            createSegment(m_startEndpoint, {cp.connectable, cp.index});
            update(this->rect());
            return;
        }
    }

    update(this->rect());
}

QPoint PanelDisplay::endpointPos(const ConnectionEndpoint& ep) const {
    return dynamic_cast<QWidget*>(ep.connectable)->pos()
           + ep.connectable->connectionPoints()[ep.index];
}

TrackSegment* PanelDisplay::createSegment(ConnectionEndpoint a, ConnectionEndpoint b) {
    TrackSegment* seg = new TrackSegment(this);
    seg->setEndpoints(endpointPos(a), endpointPos(b));
    seg->configureInteraction(!m_drawConnectionPoints);
    seg->setVisible(true);
    seg->lower();
    m_segments.push_back({seg, a, b});
    connect(seg, &TrackSegment::connectionPointsUpdated,
            this, &PanelDisplay::connectionPointsUpdated);
    return seg;
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
    for(SegmentConnection& sc : m_segments){
        sc.segment->configureInteraction(!allow_moving);
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
    if(!connection_points){
        m_connectingState = ConnectingState::NotConnecting;
    }
    for(TurnoutDisplay* td : m_turnouts){
        td->configureInteraction(!connection_points);
    }
    for(SegmentConnection& sc : m_segments){
        sc.segment->configureInteraction(!connection_points);
    }
    update(this->rect());
}

void PanelDisplay::setName(QString name){
    m_name = name;
}

QString PanelDisplay::getName(){
    return m_name;
}
