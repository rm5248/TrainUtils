/* SPDX-License-Identifier: GPL-2.0 */
#include "loconetslotmonitormodel.h"

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

    static const QStringList headerValues = {
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

    return headerValues[section];
}

int LoconetSlotMonitorModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    // FIXME: Implement me!
    return 0;
}

int LoconetSlotMonitorModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    // FIXME: Implement me!
    return 19;
}

QVariant LoconetSlotMonitorModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    // FIXME: Implement me!
    return QVariant();
}
