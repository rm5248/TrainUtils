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
    // void mousePressEvent(QMouseEvent* event) override;
    // void mouseMoveEvent(QMouseEvent *event) override;
    // void mouseReleaseEvent(QMouseEvent *event) override;

private:
};

#endif // TURNOUTDISPLAY_H
