/* SPDX-License-Identifier: GPL-2.0 */
#ifndef LCCNETWORKTABLEMODEL_H
#define LCCNETWORKTABLEMODEL_H

#include <QAbstractTableModel>

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

    void addNodeID(uint64_t id);
    void clear();
    uint64_t nodeIdForRow(int row);

private:
    QVector<uint64_t> m_nodeIds;
};

#endif // LCCNETWORKTABLEMODEL_H
