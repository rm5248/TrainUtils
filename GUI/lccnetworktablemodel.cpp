/* SPDX-License-Identifier: GPL-2.0 */
#include "lccnetworktablemodel.h"
#include "lcc.h"
#include "lcc/lccconnection.h"
#include "lcc-node-info.h"
#include "lcc-simple-node-info.h"

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

    char node_id_char[20];
    uint64_t node_id = m_nodeIds[index.row()];
    lcc_node_id_to_dotted_format(node_id, node_id_char, sizeof(node_id_char));

    if(m_connection){
        struct lcc_node_info* node_info = m_connection->lccNodeInfoForID(node_id);
        if(node_info != nullptr){
            struct lcc_simple_node_info* simple = lcc_node_info_get_simple(node_info);
            QString manufacturer = QString(lcc_simple_node_info_manufacturer_name(simple));
            QString model = QString(lcc_simple_node_info_model_name(simple));

            if(!manufacturer.isEmpty() || !model.isEmpty()){
                return QVariant(QString("%1 - %2 - %3").arg(manufacturer, model, QString(node_id_char)));
            }
        }
    }

    return QVariant(QString(node_id_char));
}

void LCCNetworkTableModel::setLCCConnection(std::shared_ptr<LCCConnection> lcc){
    m_connection = lcc;

    connect(m_connection.get(), &LCCConnection::nodeInformationUpdated,
            this, &LCCNetworkTableModel::nodeInformationUpdated);
}

void LCCNetworkTableModel::nodeInformationUpdated(uint64_t node_id){
    int row = m_nodeIds.indexOf(node_id);
    if(row < 0){
        return;
    }

    Q_EMIT dataChanged(index(row, 0), index(row, 0));
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
