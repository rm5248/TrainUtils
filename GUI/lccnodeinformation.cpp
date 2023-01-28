/* SPDX-License-Identifier: GPL-2.0 */
#include "lccnodeinformation.h"
#include "ui_lccnodeinformation.h"
#include "lcc/lccconnection.h"
#include "lcc.h"
#include "lcc-network-info.h"
#include "lcc-node-info.h"

LCCNodeInformation::LCCNodeInformation(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::LCCNodeInformation)
{
    ui->setupUi(this);

    std::vector<QCheckBox*> checkboxes = {
        ui->reservation,
        ui->teachingLearning,
        ui->firmwareUpgrade,
        ui->functionConfiguration,
        ui->display,
        ui->remoteButton,
        ui->datagram,
        ui->functionDescription,
        ui->simpleProtocol,
        ui->producerConsumer,
        ui->simpleNodeInfo,
        ui->firmwareUpgradeActive,
        ui->tractionControl,
        ui->memoryConfig,
        ui->identification,
        ui->abbreviatedCDI,
        ui->commandStation,
        ui->stream,
        ui->simpleTrainNode,
        ui->CDI,
    };

    for(QCheckBox* checkbox : checkboxes){
        checkbox->setAttribute(Qt::WA_TransparentForMouseEvents);
    }
}

LCCNodeInformation::~LCCNodeInformation()
{
    delete ui;
}

void LCCNodeInformation::clearInformation(){
    ui->id_label->setText( "No node selected" );
}

void LCCNodeInformation::setLCCConnection(std::shared_ptr<LCCConnection> lcc){
    m_connection = lcc;

    connect(m_connection.get(), &LCCConnection::newNodeDiscovered,
            this, &LCCNodeInformation::newNodeFound);
}

void LCCNodeInformation::setNodeID(uint64_t node_id){
    m_nodeId = node_id;
    clearAllData();
    updateAllValuesForNode();
}

void LCCNodeInformation::clearAllData(){
    ui->alias_label->clear();
    ui->id_label->clear();
}

void LCCNodeInformation::newNodeFound(uint64_t node_id){
    if(node_id != m_nodeId) return;

    updateAllValuesForNode();
}

void LCCNodeInformation::updateAllValuesForNode(){
    char id_buffer[20];
    struct lcc_node_info* node_info = m_connection->lccNodeInfoForID(m_nodeId);

    if(node_info == nullptr){
        return;
    }

    lcc_node_id_to_dotted_format(m_nodeId, id_buffer, sizeof(id_buffer));
    ui->id_label->setText(QString(id_buffer));
    ui->alias_label->setText(QString::number(lcc_node_info_get_alias(node_info), 16));
}
