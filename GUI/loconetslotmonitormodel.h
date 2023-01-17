/* SPDX-License-Identifier: GPL-2.0 */
#ifndef LOCONETSLOTMONITORMODEL_H
#define LOCONETSLOTMONITORMODEL_H

#include <QAbstractTableModel>

class LoconetSlotMonitorModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit LoconetSlotMonitorModel(QObject *parent = nullptr);

    // Header:
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    // Basic functionality:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

private:
};

#endif // LOCONETSLOTMONITORMODEL_H
