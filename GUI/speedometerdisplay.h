#ifndef SPEEDOMETERDISPLAY_H
#define SPEEDOMETERDISPLAY_H

#include <QWidget>

namespace Ui {
class SpeedometerDisplay;
}

class SpeedometerDisplay : public QWidget
{
    Q_OBJECT

public:
    explicit SpeedometerDisplay(QWidget *parent = nullptr);
    ~SpeedometerDisplay();

public Q_SLOTS:
    void incomingSpeedMeasurement(double speed);

private:
    Ui::SpeedometerDisplay *ui;
};

#endif // SPEEDOMETERDISPLAY_H
