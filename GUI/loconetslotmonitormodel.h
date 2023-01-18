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

    void updateSlot(int slotNum, int address, int speed );
    void updateSlotSpeed(int slotNum, int speed);

private:
    struct SlotData{
        int address = 0;
        int speed = 0;
        int status = 0;
        int use;
        bool consisted = 0;
        int throttle_id = 0;
        QString direction;
        bool functions[7] = {0};
    };

    std::array<SlotData,120> m_slotData;
};

#endif // LOCONETSLOTMONITORMODEL_H
