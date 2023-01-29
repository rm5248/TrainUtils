/* SPDX-License-Identifier: GPL-2.0 */
#ifndef LOCONETSWITCHCONTROL_H
#define LOCONETSWITCHCONTROL_H

#include <QWidget>
#include <memory>

namespace Ui {
class LoconetSwitchControl;
}

class LoconetConnection;

class LoconetSwitchControl : public QWidget
{
    Q_OBJECT

public:
    explicit LoconetSwitchControl(QWidget *parent = nullptr);
    ~LoconetSwitchControl();

    void setLoconetConnection(std::shared_ptr<LoconetConnection> conn);

private Q_SLOTS:
    void on_closeButton_clicked();
    void on_throwButton_clicked();

private:
    Ui::LoconetSwitchControl *ui;
    std::shared_ptr<LoconetConnection> m_conn;
};

#endif // LOCONETSWITCHCONTROL_H
