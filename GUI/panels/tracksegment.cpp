#include <algorithm>
#include <cmath>

#include <QMouseEvent>
#include <QPainter>

#include "tracksegment.h"

TrackSegment::TrackSegment(QWidget *parent)
    : QWidget{parent}
{
    setAttribute(Qt::WA_TranslucentBackground);
    setAutoFillBackground(false);
}

void TrackSegment::setEndpoints(QPoint parentA, QPoint parentB) {
    int x = std::min(parentA.x(), parentB.x()) - PADDING;
    int y = std::min(parentA.y(), parentB.y()) - PADDING;
    int w = std::abs(parentA.x() - parentB.x()) + 2 * PADDING;
    int h = std::abs(parentA.y() - parentB.y()) + 2 * PADDING;
    if (w < 2 * PADDING + 1) w = 2 * PADDING + 1;
    if (h < 2 * PADDING + 1) h = 2 * PADDING + 1;
    m_localA = parentA - QPoint(x, y);
    m_localB = parentB - QPoint(x, y);
    setGeometry(x, y, w, h);
    Q_EMIT connectionPointsUpdated();
}

QVector<QPoint> TrackSegment::connectionPoints() {
    return {m_localA, m_localB};
}

void TrackSegment::configureInteraction(bool interaction) {
    m_interactive = interaction;
}

QSize TrackSegment::sizeHint() const {
    return QSize(2 * PADDING + 1, 2 * PADDING + 1);
}

void TrackSegment::paintEvent(QPaintEvent* /*event*/) {
    QPainter painter(this);
    QPen pen;
    pen.setWidth(3);
    pen.setBrush(Qt::black);
    painter.setPen(pen);
    painter.drawLine(m_localA, m_localB);
}

void TrackSegment::mousePressEvent(QMouseEvent* event) {
    if (!m_interactive || event->button() == Qt::RightButton) {
        event->ignore();
        return;
    }
    if (distanceToSegment(event->pos(), m_localA, m_localB) > PADDING) {
        event->ignore();
        return;
    }
    event->accept();
}

void TrackSegment::mouseReleaseEvent(QMouseEvent* event) {
    event->ignore();
}

float TrackSegment::distanceToSegment(QPoint p, QPoint a, QPoint b) {
    QPointF pa = p - a;
    QPointF ba = b - a;
    double denom = QPointF::dotProduct(ba, ba);
    if (denom == 0.0) {
        return QLineF(QPointF(p), QPointF(a)).length();
    }
    float t = std::clamp((float)(QPointF::dotProduct(pa, ba) / denom), 0.f, 1.f);
    return QLineF(QPointF(p), QPointF(a) + t * ba).length();
}
