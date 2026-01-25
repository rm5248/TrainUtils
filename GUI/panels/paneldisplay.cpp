#include <QLayout>
#include <QPainterPath>
#include <QPainter>
#include <QHBoxLayout>
#include <QMouseEvent>

#include <log4cxx/logger.h>
#include <fmt/format.h>

#include "paneldisplay.h"
#include "panellayout.h"

static log4cxx::LoggerPtr logger = log4cxx::Logger::getLogger("traingui.PanelDisplay");

PanelDisplay::PanelDisplay(QWidget *parent)
    : QWidget{parent}
{
//    PanelLayout* pl = new PanelLayout(this);
//    QHBoxLayout* pl = new QHBoxLayout(this);
    TurnoutDisplay* td = new TurnoutDisplay(this);
    td->setGeometry(50, 50, 200, 200);
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
}
