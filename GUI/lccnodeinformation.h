/* SPDX-License-Identifier: GPL-2.0 */
#ifndef LCCNODEINFORMATION_H
#define LCCNODEINFORMATION_H

#include <QWidget>
#include <memory>

namespace Ui {
class LCCNodeInformation;
}

class LCCConnection;
class LCCNode;

class LCCNodeInformation : public QWidget
{
    Q_OBJECT

public:
    explicit LCCNodeInformation(QWidget *parent = nullptr);
    ~LCCNodeInformation();

    void setLCCConnection(std::shared_ptr<LCCConnection> lcc);
    void clearInformation();
    void setNodeID(uint64_t node_id);

private Q_SLOTS:
    void newNodeFound(uint64_t node_id);
    void nodeUpdated(uint64_t node_id);

    void on_queryProtocolsSimpleNode_clicked();

    void on_queryEventsProducedConsumed_clicked();

    void on_readCDI_clicked();

private:
    void clearAllData();
    void updateAllValuesForNode();

private:
    Ui::LCCNodeInformation *ui;
    uint64_t m_nodeId;
    std::shared_ptr<LCCConnection> m_connection;
};

#endif // LCCNODEINFORMATION_H
