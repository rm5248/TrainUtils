/* SPDX-License-Identifier: GPL-2.0 */
#ifndef THROTTLE_DISPLAY_H
#define THROTTLE_DISPLAY_H

#include <QWidget>
#include <memory>

namespace Ui {
class ThrottleDisplay;
}

class Throttle;

class ThrottleDisplay : public QWidget
{
    Q_OBJECT

public:
    explicit ThrottleDisplay(QWidget *parent = nullptr);
    ~ThrottleDisplay();

    void setThrottle(std::shared_ptr<Throttle> throttle);

private Q_SLOTS:
    void on_forwardRadio_clicked();

    void on_reverseRadio_clicked();

    void on_locoSpeed_valueChanged(int value);

    void on_selectLoco_clicked();

    void on_func0_clicked();

private:
    void configureFwdRev();

private:
    Ui::ThrottleDisplay *ui;
    std::shared_ptr<Throttle> m_throttle;
};

#endif // THROTTLE_DISPLAY_H
