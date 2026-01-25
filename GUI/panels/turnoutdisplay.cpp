#include <QPainterPath>
#include <QPainter>

#include "turnoutdisplay.h"

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
    painter.drawPath(path);
}

QSize TurnoutDisplay::sizeHint() const
{
    return QSize(200, 200);
}
