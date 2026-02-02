#ifndef PANELDISPLAY_H
#define PANELDISPLAY_H

#include <QWidget>
#include "turnoutdisplay.h"

class PanelToolsWidget;

class PanelDisplay : public QWidget
{
    Q_OBJECT
public:
    explicit PanelDisplay(QWidget *parent = nullptr);

    void setPanelToolsWidget(PanelToolsWidget* widget);

Q_SIGNALS:

protected:
    QSize sizeHint() const override;
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

private:
    QVector<TurnoutDisplay*> m_turnouts;
    QWidget* m_movingWidget = nullptr;
    QPoint m_movingWidgetStart;
    QPoint m_mouseStart;
    bool m_editing = false;
    PanelToolsWidget* m_tools = nullptr;
};

#endif // PANELDISPLAY_H
