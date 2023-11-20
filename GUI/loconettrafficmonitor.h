/* SPDX-License-Identifier: GPL-2.0 */
#ifndef LOCONETTRAFFICMONITOR_H
#define LOCONETTRAFFICMONITOR_H

#include <QWidget>

#include "loconet_buffer.h"
#include "loconet/loconetusage.h"

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

    void on_statsButton_clicked();

    void on_logStats_stateChanged(int arg1);

private:
    Ui::LoconetTrafficMonitor *ui;
    std::shared_ptr<LoconetConnection> m_connection;
    LoconetUsage m_usage;
};

#endif // LOCONETTRAFFICMONITOR_H
