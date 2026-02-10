#ifndef TURNOUTDISPLAY_H
#define TURNOUTDISPLAY_H

#include <QWidget>

#include "../common/turnout.h"

class TurnoutDisplay : public QWidget
{
    Q_OBJECT
    // Q_PROPERTY(QString traingui_turnout READ turnout WRITE setTurnout)
public:
    explicit TurnoutDisplay(QWidget *parent = nullptr);

    void setTurnout(std::shared_ptr<Turnout> turnout);

Q_SIGNALS:

protected:
    QSize sizeHint() const override;
    void paintEvent(QPaintEvent *event) override;
    // void mousePressEvent(QMouseEvent* event) override;
    // void mouseMoveEvent(QMouseEvent *event) override;
    // void mouseReleaseEvent(QMouseEvent *event) override;

private:
    void stateChanged();

private:
    std::shared_ptr<Turnout> m_turnout;
};

#endif // TURNOUTDISPLAY_H
