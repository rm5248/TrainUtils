/* SPDX-License-Identifier: GPL-2.0 */
#ifndef LOCONETTRAFFICMONITOR_H
#define LOCONETTRAFFICMONITOR_H

#include <QWidget>

#include "loconet_buffer.h"

class LoconetConnection;

namespace Ui {
class LoconetTrafficMonitor;
}

class LoconetTrafficMonitor : public QWidget
{
    Q_OBJECT

public:
    explicit LoconetTrafficMonitor(QWidget *parent = nullptr);
    ~LoconetTrafficMonitor();

    void setLoconetConnection(std::shared_ptr<LoconetConnection> conn);

private Q_SLOTS:
    void on_clearButton_clicked();
    void incomingLoconetMessage(loconet_message msg);

private:
    Ui::LoconetTrafficMonitor *ui;
    std::shared_ptr<LoconetConnection> m_connection;
};

#endif // LOCONETTRAFFICMONITOR_H
