/* SPDX-License-Identifier: GPL-2.0 */
#include "loconetslotmonitormodel.h"

static const char* headerValues[] = {
    "Slot",
    "EStop",
    "Address",
    "Speed",
    "Status",
    "Use",
    "Release",
    "Consisted",
    "Throttle ID",
    "Dir",
    "F0",
    "F1",
    "F2",
    "F3",
    "F4",
    "F5",
    "F6",
    "F7",
    "F8"
};

LoconetSlotMonitorModel::LoconetSlotMonitorModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

QVariant LoconetSlotMonitorModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    // FIXME: Implement me!
    if(role != Qt::DisplayRole){
        return QVariant();
    }

    if(orientation == Qt::Vertical){
        return QVariant();
    }

    return QVariant(headerValues[section]);
}

int LoconetSlotMonitorModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    // FIXME: Implement me!
    return m_slotData.size();
}

int LoconetSlotMonitorModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    // FIXME: Implement me!
    return sizeof(headerValues) / sizeof(headerValues[0]);
}

QVariant LoconetSlotMonitorModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if(role != Qt::DisplayRole){
        return QVariant();
    }

    SlotData data = m_slotData[index.row()];

    switch(index.column()){
    case 0:
        return QVariant(index.row());
    case 1:
        break;
    case 2:
        return QVariant(data.address);
    case 3:
        return QVariant(data.speed);
    case 4:
        break;
    case 5:
        switch(data.use){
        case LN_SLOT_STATUS_FREE: return QVariant("Free");
        case LN_SLOT_STATUS_COMMON: return QVariant("Common");
        case LN_SLOT_STATUS_IDLE: return QVariant("Idle");
        case LN_SLOT_STATUS_IN_USE: return QVariant("In use");
        }
        break;
    case 6:
        break;
    case 7:
        break;
    case 8:
        break;
    case 9:
        return data.direction;
    case 10:
        break;
    case 11:
        break;
    case 12:
        break;
    case 13:
        break;
    case 14:
        break;
    case 15:
        break;
    case 16:
        break;
    case 17:
        break;
    case 18:
        break;
    case 19:
        break;
    }

    // FIXME: Implement me!
    return QVariant();
}

void LoconetSlotMonitorModel::updateSlot(int slotNum, int address, int speed, int status, loconet_slot_status use, int direction ){
    if(slotNum < 0 || slotNum > m_slotData.size()){
        return;
    }
    m_slotData[slotNum].address = address;
    m_slotData[slotNum].speed = speed;
    m_slotData[slotNum].status = status;
    m_slotData[slotNum].use = use;
    if(direction == 1){
        m_slotData[slotNum].direction = "FWD";
    }else{
        m_slotData[slotNum].direction = "REV";
    }
}

void LoconetSlotMonitorModel::updateSlotSpeed(int slotNum, int speed){
    m_slotData[slotNum].speed = speed;

    Q_EMIT dataChanged(createIndex(slotNum, 3),
                       createIndex(slotNum, 3));
}
