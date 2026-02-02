#ifndef TURNOUTDISPLAY_H
#define TURNOUTDISPLAY_H

#include <QWidget>

class TurnoutDisplay : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(QString traingui_turnout READ turnout WRITE setTurnout)
public:
    explicit TurnoutDisplay(QWidget *parent = nullptr);

    QString turnout();
    void setTurnout(QString turnout);

Q_SIGNALS:

protected:
    QSize sizeHint() const override;
    void paintEvent(QPaintEvent *event) override;
    // void mousePressEvent(QMouseEvent* event) override;
    // void mouseMoveEvent(QMouseEvent *event) override;
    // void mouseReleaseEvent(QMouseEvent *event) override;

private:
    QString m_turnout;
};

#endif // TURNOUTDISPLAY_H
