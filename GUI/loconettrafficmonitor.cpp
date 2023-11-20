/* SPDX-License-Identifier: GPL-2.0 */
#include "loconettrafficmonitor.h"
#include "ui_loconettrafficmonitor.h"
#include "loconet/loconetconnection.h"
#include "loconet_print.h"

#include <QFileDialog>

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

    m_usage.collectStats(false);

    connect(m_connection.get(), &LoconetConnection::incomingLoconetMessage,
            this, &LoconetTrafficMonitor::incomingLoconetMessage);
    connect(m_connection.get(), &LoconetConnection::incomingLoconetMessage,
            &m_usage, &LoconetUsage::incomingLoconetMessage);
}

void LoconetTrafficMonitor::incomingLoconetMessage(loconet_message msg){
    char loconet_buffer[8192];

    loconet_message_decode_as_str( loconet_buffer, sizeof(loconet_buffer), &msg, LOCONET_PRINT_FLAG_DISPLAY_BYTES );
    QString str = QString::fromUtf8(loconet_buffer);
    ui->loconetData->appendPlainText(str);
}

void LoconetTrafficMonitor::on_statsButton_clicked()
{
    QString saveFileName = QFileDialog::getSaveFileName(this, "Save stats to file");
    if(saveFileName.isNull() || saveFileName.isEmpty()){
        return;
    }

    std::filesystem::path path(saveFileName.toStdString());
    m_usage.logStatsToFile(path);
}


void LoconetTrafficMonitor::on_logStats_stateChanged(int arg1)
{
    if(arg1 == Qt::Checked){
        m_usage.collectStats(true);
    }else{
        m_usage.clear();
        m_usage.collectStats(false);
    }
}

