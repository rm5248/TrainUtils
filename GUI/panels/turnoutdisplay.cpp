#include <QPainterPath>
#include <QPainter>
#include <QMouseEvent>
#include <QTransform>

#include "turnoutdisplay.h"
#include "../common/turnout.h"

#include <log4cxx/logger.h>
#include <fmt/format.h>

static log4cxx::LoggerPtr logger = log4cxx::Logger::getLogger("traingui.TurnoutDisplay");

TurnoutDisplay::TurnoutDisplay(QWidget *parent, Qt::WindowFlags f)
    : QWidget{parent, f}
{
    resize(kBoundingSize, kBoundingSize);
    updateConnectionPoints();
}

void TurnoutDisplay::updateConnectionPoints(){
    QRect content = contentRect();
    int cw = content.width();
    int ch = content.height();

    // Mirror the same transform stack applied in paintEvent, so the
    // connection points always match what's drawn, without needing to
    // wait for an actual paint to happen.
    QTransform transform;
    transform.translate(content.left(), content.top());
    if(m_turnoutType == TurnoutType::Left){
        transform.translate(0, ch);
        transform.scale(1, -1);
    }
    if(m_rotation != 0.0){
        QPointF center(cw / 2.0, ch / 2.0);
        transform.translate(center.x(), center.y());
        transform.rotate(m_rotation);
        transform.translate(-center.x(), -center.y());
    }

    // Connection points:
    // 0 = incoming
    // 1 = normal outgoing
    // 2 = diverged outgoing
    m_connectionPoints.clear();
    m_connectionPoints.push_back(transform.map(QPoint(0, ch / 4)));
    m_connectionPoints.push_back(transform.map(QPoint(cw, ch / 4)));
    m_connectionPoints.push_back(transform.map(QPoint(cw, ch / 2 + ch / 4)));

    Q_EMIT connectionPointsUpdated();
}

QRect TurnoutDisplay::contentRect() const {
    return QRect((width() - kContentWidth) / 2, (height() - kContentHeight) / 2,
                 kContentWidth, kContentHeight);
}

void TurnoutDisplay::paintEvent(QPaintEvent *event){
    QPainter painter(this);

    QRect content = contentRect();
    int cw = content.width();
    int ch = content.height();

    // Move into the content's local coordinate space (0,0) .. (cw,ch)
    painter.translate(content.topLeft());

    if(m_turnoutType == TurnoutType::Left){
        painter.translate(0, ch);
        painter.scale(1, -1);
    }

    if(m_rotation != 0.0){
        QPointF center(cw / 2.0, ch / 2.0);
        painter.translate(center);
        painter.rotate(m_rotation);
        painter.translate(-center);
    }

    QPoint straight_start(0, ch / 4);
    QPoint straight_end(cw, ch / 4);
    painter.drawLine(straight_start, straight_end);
    // Go 1/3 of the way and draw our diverging route
    QPoint start_diverge(cw / 3, ch / 4);
    QPoint end_diverge(cw / 2 + cw / 4, ch / 2 + ch / 4);
    painter.drawLine(start_diverge, end_diverge);
    // draw the remainder of the diverging route
    QPoint diverged_start = end_diverge;
    QPoint diverged_end(cw, ch / 2 + ch / 4);
    painter.drawLine(diverged_start, diverged_end);

    painter.setTransform(QTransform());
    QFont f;
    f.setPointSize(8);
    painter.setFont(f);
    QString turnoutInfoString;
    if(m_turnout){
        switch(m_turnout->getState()){
        case Turnout::TurnoutState::Unknown:
            turnoutInfoString = "unknown";
            break;
        case Turnout::TurnoutState::Closed:
            turnoutInfoString = "closed";
            break;
        case Turnout::TurnoutState::Thrown:
            turnoutInfoString = "thrown";
            break;
        }
    }else{
        turnoutInfoString = "N/A";
    }
    painter.drawText(QPoint(content.left(), content.top() + ch / 4), turnoutInfoString);
}

QSize TurnoutDisplay::sizeHint() const
{
    return QSize(kBoundingSize, kBoundingSize);
}

void TurnoutDisplay::mousePressEvent(QMouseEvent* event){
    if(!m_interactive){
        event->ignore();
        return;
    }

    LOG4CXX_DEBUG_FMT(logger, "press button: {} pos: {},{}",
                      (int)event->button(),
                      event->pos().x(),
                      event->pos().y());
    if(event->button() == Qt::MouseButton::LeftButton){
        m_mousePressStart = QDateTime::currentDateTime();
        m_mousePressLocation = event->pos();
    }
}

void TurnoutDisplay::mouseReleaseEvent(QMouseEvent* event){
    if(!m_interactive){
        event->ignore();
        return;
    }

    LOG4CXX_DEBUG_FMT(logger, "release button: {} pos: {},{}",
                      (int)event->button(),
                      event->pos().x(),
                      event->pos().y());

    if(event->button() != Qt::MouseButton::LeftButton){
        return;
    }

    // make sure that we're not trying to move anything.
    // if not, toggle the turnout
    int absX = std::abs(m_mousePressLocation.x() - event->pos().x());
    int absY = std::abs(m_mousePressLocation.y() - event->pos().y());
    int64_t msecs_diff = m_mousePressStart.msecsTo(QDateTime::currentDateTime());
    if((absX < 10) &&
        (absY < 10) &&
        (msecs_diff < 1000) &&
        (msecs_diff > 0) &&
        m_turnout){
        LOG4CXX_DEBUG_FMT(logger, "Toggle turnout");
        m_turnout->toggleTurnout();
    }
}

// void TurnoutDisplay::mouseMoveEvent(QMouseEvent* event){
// }


void TurnoutDisplay::setTurnout(std::shared_ptr<Turnout> turnout){
    m_turnout = turnout;

    connect(m_turnout.get(), &Turnout::stateChanged,
            this, &TurnoutDisplay::stateChanged);
}

void TurnoutDisplay::stateChanged(){
    update();
}

void TurnoutDisplay::configureInteraction(bool interaction){
    m_interactive = interaction;
}

TurnoutDisplay::TurnoutType TurnoutDisplay::turnoutType(){
    return m_turnoutType;
}

QString TurnoutDisplay::name() const{
    return m_name;
}

void TurnoutDisplay::setName(QString name){
    if(name != m_name){
        m_name = name;
        Q_EMIT nameChanged();
    }
}

void TurnoutDisplay::setTurnoutType(TurnoutType type){
    LOG4CXX_DEBUG(logger, "Turnout type changed");
    m_turnoutType = type;
    updateConnectionPoints();
    update(this->rect());
}

double TurnoutDisplay::rotation() const{
    return m_rotation;
}

void TurnoutDisplay::setRotation(double degrees){
    if(degrees != m_rotation){
        m_rotation = degrees;
        updateConnectionPoints();
        update(this->rect());
    }
}

QVector<QPoint> TurnoutDisplay::connectionPoints(){
    return m_connectionPoints;
}
