/* SPDX-License-Identifier: GPL-2.0 */
#include "loconettrafficmonitor.h"
#include "ui_loconettrafficmonitor.h"
#include "loconet/loconetconnection.h"

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

    connect(m_connection.get(), &LoconetConnection::incomingRawPacket,
            this, &LoconetTrafficMonitor::incomingRawData);
}

void LoconetTrafficMonitor::incomingRawData(QByteArray ba){
    QString data = "[";

    for(uint8_t byte : ba){
        data += QString("%1 ").arg(byte, 2, 16, QChar('0'));
    }
    data = data.trimmed();
    data += "]";

    ui->loconetData->appendPlainText(data);
}
