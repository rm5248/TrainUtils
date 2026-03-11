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
    static float distanceToSegment(QPoint p, QPoint a, QPoint b);
    QPoint m_localA;
    QPoint m_localB;
    bool m_interactive = true;
    static constexpr int PADDING = 8;
};

#endif // TRACKSEGMENT_H
