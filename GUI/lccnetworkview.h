/* SPDX-License-Identifier: GPL-2.0 */
#ifndef LCCNETWORKVIEW_H
#define LCCNETWORKVIEW_H

#include <QWidget>
#include <memory>

#include "lccnetworktablemodel.h"

namespace Ui {
class LCCNetworkView;
}

class LCCConnection;
struct lcc_node_info;

class LCCNetworkView : public QWidget
{
    Q_OBJECT

public:
    explicit LCCNetworkView(QWidget *parent = nullptr);
    ~LCCNetworkView();

    void setLCCConnection(std::shared_ptr<LCCConnection> lcc);

Q_SIGNALS:
    /**
     * Emitted when the user wants to configure the given node.
     */
    void configureNodeRequested(uint64_t node_id);

private Q_SLOTS:
    void newNodeFound(uint64_t node_id);
    void on_queryNetwork_clicked();
    void selectedNodeUpdated(const QModelIndex &current, const QModelIndex &previous);

private:
    void resetNodeInfoPane();

private:
    Ui::LCCNetworkView *ui;
    std::shared_ptr<LCCConnection> m_connection;
    LCCNetworkTableModel m_tableModel;
};

#endif // LCCNETWORKVIEW_H
