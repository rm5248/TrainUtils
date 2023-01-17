/* SPDX-License-Identifier: GPL-2.0 */
#include "loconetslotmonitor.h"
#include "ui_loconetslotmonitor.h"

LoconetSlotMonitor::LoconetSlotMonitor(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::LoconetSlotMonitor)
{
    ui->setupUi(this);
    ui->slotsTable->setModel(&m_tableModel);
}

LoconetSlotMonitor::~LoconetSlotMonitor()
{
    delete ui;
}

void LoconetSlotMonitor::setLoconetConnection(std::shared_ptr<LoconetConnection> conn){
    m_loconetConnection = conn;
}
