/* SPDX-License-Identifier: GPL-2.0 */
#include "lccnetworktablemodel.h"
#include "lcc.h"

LCCNetworkTableModel::LCCNetworkTableModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

QVariant LCCNetworkTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(role != Qt::DisplayRole){
        return QVariant();
    }

    if(orientation == Qt::Horizontal){
        return "Node ID";
    }

    return QVariant();
}

int LCCNetworkTableModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    // FIXME: Implement me!
    return m_nodeIds.length();
}

int LCCNetworkTableModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return 1;
}

QVariant LCCNetworkTableModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if(role != Qt::DisplayRole){
        return QVariant();
    }

    // FIXME: Implement me!
    char node_id_char[20];
    lcc_node_id_to_dotted_format(m_nodeIds[index.row()], node_id_char, sizeof(node_id_char));
    return QVariant(QString(node_id_char));
}

void LCCNetworkTableModel::addNodeID(uint64_t id){
    beginInsertRows(index(m_nodeIds.size(),0), m_nodeIds.size(), m_nodeIds.size());
    m_nodeIds.push_back(id);
    endInsertRows();
}

void LCCNetworkTableModel::clear(){
    beginResetModel();
    m_nodeIds.clear();
    endResetModel();
}

uint64_t LCCNetworkTableModel::nodeIdForRow(int row){
    return m_nodeIds[row];
}
