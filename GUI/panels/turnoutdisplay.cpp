#include <QPainterPath>
#include <QPainter>
#include <QMouseEvent>

#include "turnoutdisplay.h"

#include <log4cxx/logger.h>
#include <fmt/format.h>

static log4cxx::LoggerPtr logger = log4cxx::Logger::getLogger("traingui.TurnoutDisplay");

TurnoutDisplay::TurnoutDisplay(QWidget *parent)
    : QWidget{parent}
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


    painter.drawLine(0, this->height() / 4, this->width(), this->height() / 4);
    // Go 1/3 of the way and draw our diverging route
    painter.drawLine(this->width() / 3, this->height() / 4,
                     this->width() / 2 + this->width() / 4, this->height() / 2 + this->height() / 4);
    // draw the remainder of the diverging route
    painter.drawLine(this->width() / 2 + this->width() / 4, this->height() / 2 + this->height() / 4,
                    this->width(), this->height() / 2 + this->height() / 4);

    QFont f;
    f.setPointSize(8);
    painter.setFont(f);
    painter.drawText(QPoint(0, this->height() / 4), "Turnout");
}

QSize TurnoutDisplay::sizeHint() const
{
    return QSize(50, 50);
}

// void TurnoutDisplay::mousePressEvent(QMouseEvent* event){
//     LOG4CXX_DEBUG_FMT(logger, "press button: {} pos: {},{}",
//                       (int)event->button(),
//                       event->pos().x(),
//                       event->pos().y());
// }

// void TurnoutDisplay::mouseReleaseEvent(QMouseEvent* event){
//     LOG4CXX_DEBUG_FMT(logger, "release button: {} pos: {},{}",
//                       (int)event->button(),
//                       event->pos().x(),
//                       event->pos().y());
// }

// void TurnoutDisplay::mouseMoveEvent(QMouseEvent* event){
// }


void TurnoutDisplay::setTurnout(std::shared_ptr<Turnout> turnout){
    m_turnout = turnout;

    connect(m_turnout.get(), &Turnout::stateChanged,
            this, &TurnoutDisplay::stateChanged);
}

void TurnoutDisplay::stateChanged(){
    repaint();
}
