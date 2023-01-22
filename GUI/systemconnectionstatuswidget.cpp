/* SPDX-License-Identifier: GPL-2.0 */
#include "systemconnectionstatuswidget.h"
#include "ui_systemconnectionstatuswidget.h"

SystemConnectionStatusWidget::SystemConnectionStatusWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SystemConnectionStatusWidget)
{
    ui->setupUi(this);
}

SystemConnectionStatusWidget::~SystemConnectionStatusWidget()
{
    delete ui;
}

void SystemConnectionStatusWidget::setSystemConnection(std::shared_ptr<SystemConnection> conn){
    m_conn = conn;

    connect(m_conn.get(), &SystemConnection::systemNameChanged,
            this, &SystemConnectionStatusWidget::systemChanged);
    connect(m_conn.get(), &SystemConnection::isConnectedChanged,
            this, &SystemConnectionStatusWidget::systemChanged);

    systemChanged();
}

void SystemConnectionStatusWidget::systemChanged(){
    ui->label->setText( QString("%1:%2").arg( m_conn->name() ).arg( m_conn->isConnected() ? "Connected" : "Disconnected" ) );
}
