#ifndef PANELDISPLAY_H
#define PANELDISPLAY_H

#include <QWidget>
#include "turnoutdisplay.h"
#include "tracksegment.h"

class PanelToolsWidget;

class PanelDisplay : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(QString getName READ getName WRITE setName)

public:
    explicit PanelDisplay(QWidget *parent = nullptr);

    void setPanelToolsWidget(PanelToolsWidget* widget);

    void addTurnout(std::shared_ptr<Turnout> turnout);

    void setName(QString name);
    QString getName();

Q_SIGNALS:

protected:
    QSize sizeHint() const override;
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

public Q_SLOTS:
    void allowMovingChanged(bool allow_moving);
    void drawConnectionPointsChanged(bool connection_points);
    void addBlankTurnout();
    void connectionPointsUpdated();
    void setPanelSize(QSize size);

private:
    static constexpr int kGridSize = 10;
    static constexpr int kRotateHandleRadius = 6;
    static constexpr int kRotateHandleDistance = 20;
    static constexpr double kRotationSnapDegrees = 5.0;

    enum class ConnectingState{
        NotConnecting,
        Connecting,
    };

    struct ConnectionEndpoint {
        Connectable* connectable = nullptr;
        int index = -1;
    };

    // Cached per-frame list rebuilt during paintEvent
    struct CachedConnectionPoint {
        Connectable* connectable;
        int index;
        QPoint center; // in PanelDisplay coordinates
    };

    struct SegmentConnection {
        TrackSegment* segment;
        ConnectionEndpoint a;
        ConnectionEndpoint b;
    };

    QPoint endpointPos(const ConnectionEndpoint& ep) const;
    TrackSegment* createSegment(ConnectionEndpoint a, ConnectionEndpoint b);
    void updateAttachedSegments(Connectable* connectable);
    QPoint rotateHandlePos(TurnoutDisplay* td) const;

    QVector<TurnoutDisplay*> m_turnouts;
    QVector<SegmentConnection> m_segments;
    QWidget* m_selectedWidget = nullptr;
    QPoint m_movingWidgetStart;
    QPoint m_mouseStart;
    TurnoutDisplay* m_rotatingWidget = nullptr;
    QPoint m_rotationCenter;
    bool m_editing = false;
    PanelToolsWidget* m_tools = nullptr;
    bool m_allowMoving = false;
    bool m_drawConnectionPoints = false;
    QVector<CachedConnectionPoint> m_connectionPoints;
    ConnectingState m_connectingState = ConnectingState::NotConnecting;
    ConnectionEndpoint m_startEndpoint;
    QPoint m_currentMousePos;

    // non-GUI properties
    QString m_name;
    QSize m_panelSize = QSize(1280, 800);
};

#endif // PANELDISPLAY_H
