#ifndef PANELDISPLAY_H
#define PANELDISPLAY_H

#include <QWidget>
#include "turnoutdisplay.h"

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

public Q_SLOTS:
    void allowMovingChanged(bool allow_moving);
    void drawConnectionPointsChanged(bool connection_points);
    void addBlankTurnout();
    void connectionPointsUpdated();

private:
    enum class ConnectingState{
        NotConnecting,
        Connecting,
    };

    QVector<TurnoutDisplay*> m_turnouts;
    QWidget* m_selectedWidget = nullptr;
    QPoint m_movingWidgetStart;
    QPoint m_mouseStart;
    bool m_editing = false;
    PanelToolsWidget* m_tools = nullptr;
    bool m_allowMoving = false;
    bool m_drawConnectionPoints = false;
    QVector<QPoint> m_connectionPoints;
    ConnectingState m_connectingState = ConnectingState::NotConnecting;
    QPoint m_startConnectingPoint;

    // non-GUI properties
    QString m_name;
};

#endif // PANELDISPLAY_H
