#include <algorithm>
#include <cmath>
#include <limits>

#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>

#include "tracksegment.h"

TrackSegment::TrackSegment(QWidget *parent)
    : QWidget{parent}
{
    setAttribute(Qt::WA_TranslucentBackground);
    setAutoFillBackground(false);
}

void TrackSegment::setEndpoints(QPoint parentA, QPoint parentB) {
    m_parentA = parentA;
    m_parentB = parentB;
    // While the curve hasn't been manually shaped, keep it exactly straight
    // by recomputing the control points fresh every time an endpoint moves,
    // rather than dragging along stale offsets from a now-stale line.
    if(m_straight){
        straightenControlOffsets();
    }
    updateGeometry();
    Q_EMIT connectionPointsUpdated();
}

QPoint TrackSegment::endpointA() const {
    return m_parentA;
}

QPoint TrackSegment::endpointB() const {
    return m_parentB;
}

QPoint TrackSegment::controlPointA() const {
    return m_parentA + m_controlOffsetA;
}

QPoint TrackSegment::controlPointB() const {
    return m_parentB + m_controlOffsetB;
}

void TrackSegment::setControlPointA(QPoint parentPoint) {
    m_straight = false;
    m_controlOffsetA = parentPoint - m_parentA;
    updateGeometry();
}

void TrackSegment::setControlPointB(QPoint parentPoint) {
    m_straight = false;
    m_controlOffsetB = parentPoint - m_parentB;
    updateGeometry();
}

bool TrackSegment::isStraight() const {
    return m_straight;
}

void TrackSegment::makeStraight() {
    m_straight = true;
    straightenControlOffsets();
    updateGeometry();
}

void TrackSegment::straightenControlOffsets() {
    QPoint diff = m_parentB - m_parentA;
    m_controlOffsetA = diff / 3;
    m_controlOffsetB = -diff / 3;
}

void TrackSegment::updateGeometry() {
    QPoint controlA = controlPointA();
    QPoint controlB = controlPointB();

    int minX = std::min({m_parentA.x(), m_parentB.x(), controlA.x(), controlB.x()}) - PADDING;
    int minY = std::min({m_parentA.y(), m_parentB.y(), controlA.y(), controlB.y()}) - PADDING;
    int maxX = std::max({m_parentA.x(), m_parentB.x(), controlA.x(), controlB.x()}) + PADDING;
    int maxY = std::max({m_parentA.y(), m_parentB.y(), controlA.y(), controlB.y()}) + PADDING;

    int w = std::max(maxX - minX, 2 * PADDING + 1);
    int h = std::max(maxY - minY, 2 * PADDING + 1);

    m_localA = m_parentA - QPoint(minX, minY);
    m_localB = m_parentB - QPoint(minX, minY);
    m_localControlA = controlA - QPoint(minX, minY);
    m_localControlB = controlB - QPoint(minX, minY);

    setGeometry(minX, minY, w, h);
    update();
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
    painter.setRenderHint(QPainter::Antialiasing, true);

    QPen pen;
    pen.setWidth(3);
    pen.setBrush(Qt::black);
    painter.setPen(pen);

    QPainterPath path(m_localA);
    path.cubicTo(m_localControlA, m_localControlB, m_localB);
    painter.drawPath(path);
}

void TrackSegment::mousePressEvent(QMouseEvent* event) {
    if (!m_interactive || event->button() == Qt::RightButton) {
        event->ignore();
        return;
    }
    if (distanceToCurve(event->pos()) > PADDING) {
        event->ignore();
        return;
    }
    event->accept();
}

bool TrackSegment::hitTest(QPoint parentPoint) const {
    return distanceToCurve(parentPoint - pos()) <= PADDING;
}

void TrackSegment::mouseReleaseEvent(QMouseEvent* event) {
    event->ignore();
}

QPointF TrackSegment::bezierPoint(float t) const {
    float u = 1.0f - t;
    QPointF a(m_localA);
    QPointF b(m_localControlA);
    QPointF c(m_localControlB);
    QPointF d(m_localB);
    return u * u * u * a + 3 * u * u * t * b + 3 * u * t * t * c + t * t * t * d;
}

float TrackSegment::distanceToCurve(QPoint p) const {
    constexpr int kSamples = 24;
    QPoint prev = m_localA;
    float minDist = std::numeric_limits<float>::max();
    for(int i = 1; i <= kSamples; i++){
        QPoint next = bezierPoint(float(i) / kSamples).toPoint();
        minDist = std::min(minDist, distanceToSegment(p, prev, next));
        prev = next;
    }
    return minDist;
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
