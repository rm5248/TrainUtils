/* SPDX-License-Identifier: GPL-2.0 */
#ifndef LCCTRAFFICMONITOR_H
#define LCCTRAFFICMONITOR_H

#include <QWidget>

#include "lcc/lccconnection.h"

namespace Ui {
class LCCTrafficMonitor;
}

class LCCTrafficMonitor : public QWidget
{
    Q_OBJECT

public:
    explicit LCCTrafficMonitor(QWidget *parent = nullptr);
    ~LCCTrafficMonitor();

    void setLCCConnection(std::shared_ptr<LCCConnection> lcc);

private Q_SLOTS:
    void incomingFrame(lcc_can_frame* frame);

    void on_clearButton_clicked();

private:
    Ui::LCCTrafficMonitor *ui;
    std::shared_ptr<LCCConnection> m_lcc;
};

#endif // LCCTRAFFICMONITOR_H
