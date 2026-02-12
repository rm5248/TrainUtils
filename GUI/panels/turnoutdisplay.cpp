#include <QPainterPath>
#include <QPainter>
#include <QMouseEvent>

#include "turnoutdisplay.h"
#include "../common/turnout.h"

#include <log4cxx/logger.h>
#include <fmt/format.h>

static log4cxx::LoggerPtr logger = log4cxx::Logger::getLogger("traingui.TurnoutDisplay");

TurnoutDisplay::TurnoutDisplay(QWidget *parent, Qt::WindowFlags f)
    : QWidget{parent, f}
{

}

void TurnoutDisplay::paintEvent(QPaintEvent *event){
    QPainterPath path;
    path.moveTo(0, 100);
    path.lineTo(200, 100);

    path.moveTo(100, 0);
    path.lineTo(100, 200);

    QPainter painter(this);
    // painter.drawPath(path);

    if(m_turnoutType == TurnoutType::Left){
        painter.translate(0, this->height());
        painter.scale(1, -1);
    }

    painter.drawLine(0, this->height() / 4, this->width(), this->height() / 4);
    // Go 1/3 of the way and draw our diverging route
    painter.drawLine(this->width() / 3, this->height() / 4,
                     this->width() / 2 + this->width() / 4, this->height() / 2 + this->height() / 4);
    // draw the remainder of the diverging route
    painter.drawLine(this->width() / 2 + this->width() / 4, this->height() / 2 + this->height() / 4,
                    this->width(), this->height() / 2 + this->height() / 4);


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
    painter.drawText(QPoint(0, this->height() / 4), turnoutInfoString);
}

QSize TurnoutDisplay::sizeHint() const
{
    return QSize(50, 50);
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

void TurnoutDisplay::setTurnoutType(TurnoutType type){
    LOG4CXX_DEBUG(logger, "Turnout type changed");
    m_turnoutType = type;
    update(this->rect());
}
