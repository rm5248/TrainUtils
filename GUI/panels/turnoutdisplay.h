#ifndef TURNOUTDISPLAY_H
#define TURNOUTDISPLAY_H

#include <QWidget>
#include <QDateTime>

#include "../common/turnout.h"

class TurnoutDisplay : public QWidget
{
    Q_OBJECT
    // Q_PROPERTY(QString traingui_turnout READ turnout WRITE setTurnout)
public:
    explicit TurnoutDisplay(QWidget *parent = nullptr);

    void setTurnout(std::shared_ptr<Turnout> turnout);

Q_SIGNALS:

public Q_SLOTS:
    void configureInteraction(bool interaction);

protected:
    QSize sizeHint() const override;
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent* event) override;
    // void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    void stateChanged();

private:
    std::shared_ptr<Turnout> m_turnout;
    bool m_interactive = true;
    QDateTime m_mousePressStart;
    QPoint m_mousePressLocation;
};

#endif // TURNOUTDISPLAY_H
