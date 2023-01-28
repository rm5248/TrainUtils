/* SPDX-License-Identifier: GPL-2.0 */
#include "lccnetworkview.h"
#include "ui_lccnetworkview.h"
#include "lcc/lccconnection.h"
#include "lcc-node-info.h"
#include "lccnodeinformation.h"

#include <log4cxx/logger.h>
#include <fmt/core.h>

static log4cxx::LoggerPtr logger = log4cxx::Logger::getLogger( "traingui.LCCNetworkView" );

LCCNetworkView::LCCNetworkView(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::LCCNetworkView)
{
    ui->setupUi(this);

    resetNodeInfoPane();

    ui->lccNetwork->setModel(&m_tableModel);
    ui->lccNetwork->horizontalHeader()->setStretchLastSection(true);

    connect( ui->lccNetwork->selectionModel(), &QItemSelectionModel::currentChanged,
             this, &LCCNetworkView::selectedNodeUpdated);
}

LCCNetworkView::~LCCNetworkView()
{
    delete ui;
}

void LCCNetworkView::setLCCConnection(std::shared_ptr<LCCConnection> lcc){
    m_connection = lcc;
    ui->selectedNode->setLCCConnection(lcc);

    connect(m_connection.get(), &LCCConnection::newNodeDiscovered,
            this, &LCCNetworkView::newNodeFound);
}

void LCCNetworkView::newNodeFound(uint64_t node_id){
    // Notify our model that there is a new node
    char node_buffer[20];
    lcc_node_id_to_dotted_format(node_id, node_buffer, sizeof(node_buffer));
    LOG4CXX_DEBUG_FMT(logger, "Found node {}", node_buffer);
    m_tableModel.addNodeID(node_id);
}

void LCCNetworkView::on_queryNetwork_clicked()
{
    resetNodeInfoPane();
    m_tableModel.clear();
    m_connection->refreshNetwork();
}


void LCCNetworkView::selectedNodeUpdated(const QModelIndex &current, const QModelIndex &previous)
{
    LOG4CXX_DEBUG_FMT( logger, "Selected updated: {},{}", current.row(), current.column());

    // Now that the selected node has been updated, update our information panel
    ui->selectedNode->setNodeID(m_tableModel.nodeIdForRow(current.row()));
}

void LCCNetworkView::resetNodeInfoPane(){
    ui->selectedNode->clearInformation();
}

