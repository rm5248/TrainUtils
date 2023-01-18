/* SPDX-License-Identifier: GPL-2.0 */
#ifndef LOCONETSLOTMONITOR_H
#define LOCONETSLOTMONITOR_H

#include <QWidget>

#include "loconetslotmonitormodel.h"

#include "loconet_buffer.h"

class LoconetConnection;

namespace Ui {
class LoconetSlotMonitor;
}

class LoconetSlotMonitor : public QWidget
{
    Q_OBJECT

public:
    explicit LoconetSlotMonitor(QWidget *parent = nullptr);
    ~LoconetSlotMonitor();

    void setLoconetConnection(std::shared_ptr<LoconetConnection> conn);

private Q_SLOTS:
    void incomingLoconetMessage(loconet_message msg);

    void on_forceRefresh_clicked();

private:
    Ui::LoconetSlotMonitor *ui;
    LoconetSlotMonitorModel m_tableModel;
    std::shared_ptr<LoconetConnection> m_loconetConnection;
};

#endif // LOCONETSLOTMONITOR_H
