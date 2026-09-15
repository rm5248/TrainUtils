#ifndef TRACKSEGMENT_H
#define TRACKSEGMENT_H

#include <QWidget>

#include "connectable.h"

class TrackSegment : public QWidget, public Connectable
{
    Q_OBJECT
public:
    explicit TrackSegment(QWidget *parent = nullptr);
    void setEndpoints(QPoint parentA, QPoint parentB);
    QVector<QPoint> connectionPoints() override;

    // Endpoints and bezier control points, all in the parent's (panel's)
    // coordinate space.
    QPoint endpointA() const;
    QPoint endpointB() const;
    QPoint controlPointA() const;
    QPoint controlPointB() const;
    void setControlPointA(QPoint parentPoint);
    void setControlPointB(QPoint parentPoint);

    // Whether the curve is currently following the endpoints as a straight
    // line (true), or has been manually shaped by dragging a control point
    // (false). makeStraight() resets it back to following the endpoints.
    bool isStraight() const;
    void makeStraight();

    // True if parentPoint (in the parent's coordinate space) is close
    // enough to the rendered curve to count as a click on it. Used so a
    // click meant for the segment isn't lost to an overlapping sibling's
    // (e.g. a turnout's) bounding box.
    bool hitTest(QPoint parentPoint) const;

Q_SIGNALS:
    void connectionPointsUpdated();

public Q_SLOTS:
    void configureInteraction(bool interaction);

protected:
    QSize sizeHint() const override;
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    void updateGeometry();
    void straightenControlOffsets();
    QPointF bezierPoint(float t) const;
    float distanceToCurve(QPoint p) const;
    static float distanceToSegment(QPoint p, QPoint a, QPoint b);

    QPoint m_parentA;
    QPoint m_parentB;
    // Control point positions, stored as offsets from their governing
    // endpoint so that they move rigidly with it as the segment is
    // reconnected (e.g. when a turnout it's attached to moves or rotates).
    // Only meaningful once m_straight is false -- while straight, they're
    // recomputed fresh on every setEndpoints() call instead.
    QPoint m_controlOffsetA;
    QPoint m_controlOffsetB;
    bool m_straight = true;

    QPoint m_localA;
    QPoint m_localB;
    QPoint m_localControlA;
    QPoint m_localControlB;

    bool m_interactive = true;
    static constexpr int PADDING = 8;
};

#endif // TRACKSEGMENT_H
