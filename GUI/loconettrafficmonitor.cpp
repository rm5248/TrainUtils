/* SPDX-License-Identifier: GPL-2.0 */
#include "loconettrafficmonitor.h"
#include "ui_loconettrafficmonitor.h"
#include "loconet/loconetconnection.h"
#include "loconet_print.h"

LoconetTrafficMonitor::LoconetTrafficMonitor(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::LoconetTrafficMonitor)
{
    ui->setupUi(this);
}

LoconetTrafficMonitor::~LoconetTrafficMonitor()
{
    delete ui;
}

void LoconetTrafficMonitor::on_clearButton_clicked()
{
    ui->loconetData->clear();
}

void LoconetTrafficMonitor::setLoconetConnection(std::shared_ptr<LoconetConnection> conn){
    m_connection = conn;

    connect(m_connection.get(), &LoconetConnection::incomingLoconetMessage,
            this, &LoconetTrafficMonitor::incomingLoconetMessage);
}

void LoconetTrafficMonitor::incomingLoconetMessage(loconet_message msg){
    char loconet_buffer[8192];

    loconet_message_decode_as_str( loconet_buffer, sizeof(loconet_buffer), &msg, LOCONET_PRINT_FLAG_DISPLAY_BYTES );
    QString str = QString::fromUtf8(loconet_buffer);
    ui->loconetData->appendPlainText(str);
}
