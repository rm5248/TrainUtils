#ifndef TURNOUTDISPLAY_H
#define TURNOUTDISPLAY_H

#include <QWidget>

class TurnoutDisplay : public QWidget
{
    Q_OBJECT
public:
    explicit TurnoutDisplay(QWidget *parent = nullptr);

Q_SIGNALS:

protected:
    QSize sizeHint() const override;
    void paintEvent(QPaintEvent *event) override;

};

#endif // TURNOUTDISPLAY_H
