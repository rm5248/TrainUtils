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
        bool functions[9];
        int throttle_id = (slot_data.id1 << 8) | slot_data.id2;

        functions[0] = slot_data.dir_funcs & (0x01 << 4);
        functions[1] = slot_data.dir_funcs & (0x01 << 0);
        functions[2] = slot_data.dir_funcs & (0x01 << 1);
        functions[3] = slot_data.dir_funcs & (0x01 << 2);
        functions[4] = slot_data.dir_funcs & (0x01 << 3);
        functions[5] = slot_data.sound & (0x01 << 0);
        functions[6] = slot_data.sound & (0x01 << 1);
        functions[7] = slot_data.sound & (0x01 << 2);
        functions[8] = slot_data.sound & (0x01 << 3);

        m_tableModel.updateSlot(slotNum,
                                addr,
                                slot_data.speed,
                                0,
                                slotStatus,
                                (slot_data.dir_funcs & (0x01 << 5)) ? 1 : 0,
                                functions,
                                throttle_id);

        if(slotStatus == LN_SLOT_STATUS_FREE){
            ui->slotsTable->hideRow(slotNum);
        }else{
            ui->slotsTable->showRow(slotNum);
        }
    }else if(msg.opcode == LN_OPC_LOCO_SPEED){
        m_tableModel.updateSlotSpeed(msg.speed.slot, msg.speed.speed);
    }else if(msg.opcode == LN_OPC_LOCO_DIR_FUNC){
        m_tableModel.updateSlotDirection(msg.direction_functions.slot, msg.direction_functions.dir_funcs & (0x01 << 5) ? 1 : 0);
        m_tableModel.updateSlotFunction(msg.direction_functions.slot, 0, msg.direction_functions.dir_funcs & (0x01 << 4) ? 1 : 0);
        m_tableModel.updateSlotFunction(msg.direction_functions.slot, 1, msg.direction_functions.dir_funcs & (0x01 << 0) ? 1 : 0);
        m_tableModel.updateSlotFunction(msg.direction_functions.slot, 2, msg.direction_functions.dir_funcs & (0x01 << 1) ? 1 : 0);
        m_tableModel.updateSlotFunction(msg.direction_functions.slot, 3, msg.direction_functions.dir_funcs & (0x01 << 2) ? 1 : 0);
        m_tableModel.updateSlotFunction(msg.direction_functions.slot, 4, msg.direction_functions.dir_funcs & (0x01 << 3) ? 1 : 0);
    }else if(msg.opcode == LN_OPC_LOCO_SOUND){
        m_tableModel.updateSlotFunction(msg.sound.slot, 5, msg.sound.snd & (0x01 << 0) ? 1 : 0);
        m_tableModel.updateSlotFunction(msg.sound.slot, 6, msg.sound.snd & (0x01 << 1) ? 1 : 0);
        m_tableModel.updateSlotFunction(msg.sound.slot, 7, msg.sound.snd & (0x01 << 2) ? 1 : 0);
        m_tableModel.updateSlotFunction(msg.sound.slot, 8, msg.sound.snd & (0x01 << 3) ? 1 : 0);
    }
}

void LoconetSlotMonitor::on_forceRefresh_clicked()
{
    // For each slot, query it!
    loconet_message msg;
    for( int slotNum = 0; slotNum < 120; slotNum++ ){
        msg.opcode = LN_OPC_REQUEST_SLOT_DATA;
        msg.req_slot_data.nul = 0;
        msg.req_slot_data.slot = slotNum;

        m_loconetConnection->sendMessage(msg);
    }
}

