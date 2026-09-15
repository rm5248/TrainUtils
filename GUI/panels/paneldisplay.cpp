#include <cmath>
#include <numbers>

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
    setFixedSize(m_panelSize);
}

void PanelDisplay::paintEvent(QPaintEvent *event){
    QWidget::paintEvent(event);
    QPainter painter(this);

    // Draw border around the panel
    {
        QPen borderPen;
        borderPen.setWidth(2);
        borderPen.setBrush(Qt::black);
        borderPen.setStyle(Qt::SolidLine);
        painter.setPen(borderPen);
        painter.drawRect(rect().adjusted(0, 0, -1, -1));
    }

    if(m_selectedWidget){
        QPen pen;
        pen.setWidth(3);
        pen.setBrush(Qt::green);
        pen.setStyle(Qt::DashDotLine);
        painter.setPen(pen);
        painter.setBrush(Qt::NoBrush);

        TurnoutDisplay* rotatable = dynamic_cast<TurnoutDisplay*>(m_selectedWidget);
        if(rotatable){
            // Draw a box around the turnout's actual (rotated) artwork,
            // not its padded, always-axis-aligned widget bounds.
            QRect content = rotatable->contentRect();

            painter.save();
            painter.translate(rotatable->pos());
            painter.translate(content.center());
            painter.rotate(rotatable->rotation());
            painter.translate(-content.center());
            painter.drawRect(content);
            painter.restore();

            QPoint center = rotatable->pos() + content.center();
            QPoint handlePos = rotateHandlePos(rotatable);

            QPen handleLinePen;
            handleLinePen.setWidth(1);
            handleLinePen.setBrush(Qt::darkGray);
            handleLinePen.setStyle(Qt::DashLine);
            painter.setPen(handleLinePen);
            painter.drawLine(center, handlePos);

            QPen handlePen;
            handlePen.setWidth(2);
            handlePen.setBrush(Qt::darkGreen);
            painter.setPen(handlePen);
            painter.setBrush(Qt::white);
            painter.drawEllipse(handlePos, kRotateHandleRadius, kRotateHandleRadius);
            painter.setBrush(Qt::NoBrush);
        }else if(TrackSegment* selectedSegment = dynamic_cast<TrackSegment*>(m_selectedWidget)){
            // Draw the bezier control handles instead of a bounding box
            QPoint a = selectedSegment->endpointA();
            QPoint b = selectedSegment->endpointB();
            QPoint controlA = selectedSegment->controlPointA();
            QPoint controlB = selectedSegment->controlPointB();

            QPen handleLinePen;
            handleLinePen.setWidth(1);
            handleLinePen.setBrush(Qt::darkGray);
            handleLinePen.setStyle(Qt::DashLine);
            painter.setPen(handleLinePen);
            painter.drawLine(a, controlA);
            painter.drawLine(b, controlB);

            QPen handlePen;
            handlePen.setWidth(2);
            handlePen.setBrush(Qt::darkGreen);
            painter.setPen(handlePen);
            painter.setBrush(Qt::white);
            painter.drawEllipse(controlA, kRotateHandleRadius, kRotateHandleRadius);
            painter.drawEllipse(controlB, kRotateHandleRadius, kRotateHandleRadius);
            painter.setBrush(Qt::NoBrush);
        }else{
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

            painter.drawPath(path);
        }
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
    td->configureInteraction(!m_allowMoving && !m_drawConnectionPoints);
    td->installEventFilter(this);
    m_turnouts.push_back(td);

    connect(td, &TurnoutDisplay::connectionPointsUpdated,
            this, &PanelDisplay::connectionPointsUpdated);
}

QSize PanelDisplay::sizeHint() const
{
    return m_panelSize;
}

void PanelDisplay::setPanelSize(QSize size) {
    m_panelSize = size;
    setFixedSize(size);
}

void PanelDisplay::mousePressEvent(QMouseEvent* event){
    QWidget* widgetAtPos = childAt(event->pos());

    // A track segment's actual clickable curve can be much thinner than its
    // padded bounding box, which can also be covered by an overlapping
    // sibling (e.g. a turnout's bounding box). Prefer an actual curve hit
    // over whatever childAt() finds geometrically, so clicking a segment
    // near a turnout still selects the segment.
    for(const SegmentConnection& sc : m_segments){
        if(sc.segment->hitTest(event->pos())){
            widgetAtPos = sc.segment;
            break;
        }
    }

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

    // Check for a hit on the rotate handle of the currently selected turnout,
    // or a bezier control handle of the currently selected segment.
    if(event->button() == Qt::LeftButton && tryStartHandleDrag(event->pos())){
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

    if(m_rotatingWidget){
        QPoint delta = event->pos() - m_rotationCenter;
        // 0 degrees = straight up, positive = clockwise, matching QPainter::rotate()
        double angle = std::atan2(delta.x(), -delta.y()) * 180.0 / std::numbers::pi;
        if(event->modifiers() & Qt::ControlModifier){
            angle = std::lround(angle / kRotationSnapDegrees) * kRotationSnapDegrees;
        }
        m_rotatingWidget->setRotation(angle);
        updateAttachedSegments(m_rotatingWidget);
        update(this->rect());
        return;
    }

    if(m_draggingControlSegment){
        if(m_draggingControlIsA){
            m_draggingControlSegment->setControlPointA(event->pos());
        }else{
            m_draggingControlSegment->setControlPointB(event->pos());
        }
        update(this->rect());
        return;
    }

    if(m_selectedWidget == nullptr){
        return;
    }

    // Track segments aren't independently movable -- they only follow
    // their connected endpoints (see updateAttachedSegments).
    if(dynamic_cast<TrackSegment*>(m_selectedWidget)){
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

    // Snap to an invisible grid while Ctrl is held
    if(event->modifiers() & Qt::ControlModifier){
        newX = std::lround(newX / static_cast<double>(kGridSize)) * kGridSize;
        newY = std::lround(newY / static_cast<double>(kGridSize)) * kGridSize;
    }

    // Clamp so the widget stays within the panel bounds
    newX = std::max(0, std::min(newX, width() - m_selectedWidget->width()));
    newY = std::max(0, std::min(newY, height() - m_selectedWidget->height()));

    m_selectedWidget->setGeometry(newX, newY, m_selectedWidget->width(), m_selectedWidget->height());

    // Update any segments whose endpoints are attached to the moved widget
    Connectable* movedConnectable = dynamic_cast<Connectable*>(m_selectedWidget);
    if(movedConnectable){
        updateAttachedSegments(movedConnectable);
    }

    update(this->rect());
}

void PanelDisplay::mouseReleaseEvent(QMouseEvent* event){
    if(m_rotatingWidget && event->button() == Qt::LeftButton){
        m_rotatingWidget = nullptr;
        releaseMouse();
        update(this->rect());
        return;
    }

    if(m_draggingControlSegment && event->button() == Qt::LeftButton){
        m_draggingControlSegment = nullptr;
        releaseMouse();
        update(this->rect());
        return;
    }

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

void PanelDisplay::updateAttachedSegments(Connectable* connectable) {
    for(SegmentConnection& sc : m_segments){
        if(sc.a.connectable == connectable || sc.b.connectable == connectable){
            sc.segment->setEndpoints(endpointPos(sc.a), endpointPos(sc.b));
        }
    }
}

TrackSegment* PanelDisplay::createSegment(ConnectionEndpoint a, ConnectionEndpoint b) {
    TrackSegment* seg = new TrackSegment(this);
    seg->setEndpoints(endpointPos(a), endpointPos(b));
    seg->configureInteraction(!m_drawConnectionPoints);
    seg->setVisible(true);
    seg->lower();
    seg->installEventFilter(this);
    m_segments.push_back({seg, a, b});
    connect(seg, &TrackSegment::connectionPointsUpdated,
            this, &PanelDisplay::connectionPointsUpdated);
    return seg;
}

QPoint PanelDisplay::rotateHandlePos(TurnoutDisplay* td) const {
    QRect content = td->contentRect();
    QPoint center = td->pos() + content.center();
    double rad = td->rotation() * std::numbers::pi / 180.0;
    double dist = content.height() / 2.0 + kRotateHandleDistance;
    int dx = std::lround(std::sin(rad) * dist);
    int dy = std::lround(-std::cos(rad) * dist);
    return center + QPoint(dx, dy);
}

bool PanelDisplay::tryStartHandleDrag(QPoint panelPos) {
    if(!m_selectedWidget){
        return false;
    }

    TurnoutDisplay* rotatable = dynamic_cast<TurnoutDisplay*>(m_selectedWidget);
    if(rotatable){
        QPoint handlePos = rotateHandlePos(rotatable);
        QRect handleRect(handlePos.x() - kRotateHandleRadius, handlePos.y() - kRotateHandleRadius,
                          kRotateHandleRadius * 2, kRotateHandleRadius * 2);
        if(handleRect.contains(panelPos)){
            m_rotatingWidget = rotatable;
            m_rotationCenter = rotatable->pos() + rotatable->contentRect().center();
            grabMouse();
            return true;
        }
        return false;
    }

    TrackSegment* selectedSegment = dynamic_cast<TrackSegment*>(m_selectedWidget);
    if(selectedSegment){
        QPoint controlA = selectedSegment->controlPointA();
        QPoint controlB = selectedSegment->controlPointB();
        QRect controlARect(controlA.x() - kRotateHandleRadius, controlA.y() - kRotateHandleRadius,
                            kRotateHandleRadius * 2, kRotateHandleRadius * 2);
        QRect controlBRect(controlB.x() - kRotateHandleRadius, controlB.y() - kRotateHandleRadius,
                            kRotateHandleRadius * 2, kRotateHandleRadius * 2);
        if(controlARect.contains(panelPos)){
            m_draggingControlSegment = selectedSegment;
            m_draggingControlIsA = true;
            grabMouse();
            return true;
        }
        if(controlBRect.contains(panelPos)){
            m_draggingControlSegment = selectedSegment;
            m_draggingControlIsA = false;
            grabMouse();
            return true;
        }
    }
    return false;
}

bool PanelDisplay::eventFilter(QObject* watched, QEvent* event) {
    // Handles (the turnout rotate handle, bezier control handles) are drawn
    // by PanelDisplay itself, on top of everything, but a click there can
    // still land on a normal, interactive child widget underneath (e.g. a
    // turnout) which would otherwise consume it for its own click handling
    // before PanelDisplay::mousePressEvent ever sees it. Intercept clicks on
    // an active handle here, before the child gets a chance to.
    if(event->type() == QEvent::MouseButtonPress){
        QWidget* childWidget = qobject_cast<QWidget*>(watched);
        QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
        if(childWidget && mouseEvent->button() == Qt::LeftButton){
            QPoint panelPos = childWidget->pos() + mouseEvent->pos();
            if(tryStartHandleDrag(panelPos)){
                return true;
            }
        }
    }
    return QWidget::eventFilter(watched, event);
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
    td->configureInteraction(!m_allowMoving && !m_drawConnectionPoints);
    td->installEventFilter(this);
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
