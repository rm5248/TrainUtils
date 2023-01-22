/* SPDX-License-Identifier: GPL-2.0 */
#ifndef SYSTEMCONNECTIONSTATUSWIDGET_H
#define SYSTEMCONNECTIONSTATUSWIDGET_H

#include <QWidget>

#include "systemconnection.h"

namespace Ui {
class SystemConnectionStatusWidget;
}

class SystemConnectionStatusWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SystemConnectionStatusWidget(QWidget *parent = nullptr);
    ~SystemConnectionStatusWidget();

    void setSystemConnection(std::shared_ptr<SystemConnection> conn);

private Q_SLOTS:
    void systemChanged();

private:
    Ui::SystemConnectionStatusWidget *ui;
    std::shared_ptr<SystemConnection> m_conn;
};

#endif // SYSTEMCONNECTIONSTATUSWIDGET_H
