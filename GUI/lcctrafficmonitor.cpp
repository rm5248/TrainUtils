/* SPDX-License-Identifier: GPL-2.0 */
#include "lcctrafficmonitor.h"
#include "ui_lcctrafficmonitor.h"

#include <fmt/format.h>

LCCTrafficMonitor::LCCTrafficMonitor(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::LCCTrafficMonitor)
{
    ui->setupUi(this);
}

LCCTrafficMonitor::~LCCTrafficMonitor()
{
    delete ui;
}

void LCCTrafficMonitor::setLCCConnection(std::shared_ptr<LCCConnection> lcc){
    m_lcc = lcc;

    connect(m_lcc.get(), &LCCConnection::incomingRawFrame,
            this, &LCCTrafficMonitor::incomingFrame);
}

void LCCTrafficMonitor::incomingFrame(lcc_can_frame* frame){
    std::string data;
    int x;
    for(x = 0; x < frame->can_len; x++){
        data += fmt::format(" {:02x}", frame->data[x] );
    }
    while(x++ < 7){
        data += "   ";
    }

    std::string allData = fmt::format("[[{:x}]{}]", frame->can_id, data);
    ui->textArea->append( QString::fromStdString(allData) );
}

void LCCTrafficMonitor::on_clearButton_clicked()
{
    ui->textArea->clear();
}
