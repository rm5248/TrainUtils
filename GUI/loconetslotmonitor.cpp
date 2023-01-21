/* SPDX-License-Identifier: GPL-2.0 */
#include "loconetslotmonitor.h"
#include "ui_loconetslotmonitor.h"
#include "loconet/loconetconnection.h"

LoconetSlotMonitor::LoconetSlotMonitor(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::LoconetSlotMonitor)
{
    ui->setupUi(this);
    ui->slotsTable->setModel(&m_tableModel);

    for(int x = 0; x < m_tableModel.rowCount(); x++ ){
        ui->slotsTable->hideRow(x);
    }
}

LoconetSlotMonitor::~LoconetSlotMonitor()
{
    delete ui;
}

void LoconetSlotMonitor::setLoconetConnection(std::shared_ptr<LoconetConnection> conn){
    m_loconetConnection = conn;

    connect(m_loconetConnection.get(), &LoconetConnection::incomingLoconetMessage,
            this, &LoconetSlotMonitor::incomingLoconetMessage);

    on_forceRefresh_clicked();
}

void LoconetSlotMonitor::incomingLoconetMessage(loconet_message msg){
    if(msg.opcode == LN_OPC_SLOT_READ_DATA){
        // Update our model with the new data for the slot
        loconet_slot_data slot_data = msg.slot_data;
        loconet_slot_status slotStatus = (loconet_slot_status)LN_SLOT_STATUS(msg);
        int addr = msg.slot_data.addr1 | (msg.slot_data.addr2 << 7);
        int slotNum = slot_data.slot;

        m_tableModel.updateSlot(slotNum,
                                addr,
                                slot_data.speed,
                                0,
                                slotStatus,
                                (slot_data.dir_funcs & (0x01 << 8)) ? 1 : 0);

        if(slotStatus == LN_SLOT_STATUS_FREE){
            ui->slotsTable->hideRow(slotNum);
        }else{
            ui->slotsTable->showRow(slotNum);
        }
    }else if(msg.opcode == LN_OPC_LOCO_SPEED){
        m_tableModel.updateSlotSpeed(msg.speed.slot, msg.speed.speed);
    }
}

void LoconetSlotMonitor::on_forceRefresh_clicked()
{
    // For each slot, query it!
    loconet_message msg;
    for( int slotNum = 0; slotNum < 1; slotNum++ ){
        msg.opcode = LN_OPC_REQUEST_SLOT_DATA;
        msg.req_slot_data.nul = 0;
        msg.req_slot_data.slot = slotNum;

        m_loconetConnection->sendMessage(msg);
    }
}

