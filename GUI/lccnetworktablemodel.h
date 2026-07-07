/* SPDX-License-Identifier: GPL-2.0 */
#ifndef LCCNETWORKTABLEMODEL_H
#define LCCNETWORKTABLEMODEL_H

#include <QAbstractTableModel>
#include <memory>

class LCCConnection;

class LCCNetworkTableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit LCCNetworkTableModel(QObject *parent = nullptr);

    // Header:
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    // Basic functionality:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    void setLCCConnection(std::shared_ptr<LCCConnection> lcc);
    void addNodeID(uint64_t id);
    void clear();
    uint64_t nodeIdForRow(int row);

private Q_SLOTS:
    void nodeInformationUpdated(uint64_t node_id);

private:
    QVector<uint64_t> m_nodeIds;
    std::shared_ptr<LCCConnection> m_connection;
};

#endif // LCCNETWORKTABLEMODEL_H
