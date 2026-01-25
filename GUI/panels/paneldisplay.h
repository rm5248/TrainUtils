#ifndef PANELDISPLAY_H
#define PANELDISPLAY_H

#include <QWidget>
#include "turnoutdisplay.h"

class PanelDisplay : public QWidget
{
    Q_OBJECT
public:
    explicit PanelDisplay(QWidget *parent = nullptr);

Q_SIGNALS:

protected:
    QSize sizeHint() const override;
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent* event) override;

private:
    QVector<TurnoutDisplay*> m_turnouts;

};

#endif // PANELDISPLAY_H
