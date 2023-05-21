/* SPDX-License-Identifier: GPL-2.0 */
#include "lccnodeinformation.h"
#include "ui_lccnodeinformation.h"
#include "lcc/lccconnection.h"
#include "lcc.h"
#include "lcc-network-info.h"
#include "lcc-node-info.h"
#include "lcc/lccnode.h"

#include <log4cxx/logger.h>
#include <fmt/format.h>
#include <QXmlStreamReader>

static log4cxx::LoggerPtr logger = log4cxx::Logger::getLogger( "traingui.lcc.LCCNodeInformation" );

static std::vector<QCheckBox*> get_checkboxes(Ui::LCCNodeInformation *ui){
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

    return checkboxes;
}

LCCNodeInformation::LCCNodeInformation(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::LCCNodeInformation)
{
    ui->setupUi(this);

    std::vector<QCheckBox*> checkboxes = get_checkboxes(ui);

    for(QCheckBox* checkbox : checkboxes){
        checkbox->setAttribute(Qt::WA_TransparentForMouseEvents);
    }

    clearAllData();
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
    connect(m_connection.get(), &LCCConnection::nodeInformationUpdated,
            this, &LCCNodeInformation::nodeUpdated);
}

void LCCNodeInformation::setNodeID(uint64_t node_id){
    m_nodeId = node_id;
    clearAllData();
    updateAllValuesForNode();
}

void LCCNodeInformation::clearAllData(){
    ui->alias_label->clear();
    ui->id_label->clear();

    std::vector<QCheckBox*> checkboxes = get_checkboxes(ui);

    for(QCheckBox* checkbox : checkboxes){
        checkbox->setCheckState(Qt::Unchecked);
    }

    qDeleteAll(ui->eventsConsumedGroup->findChildren<QWidget*>("", Qt::FindDirectChildrenOnly));
    qDeleteAll(ui->eventsProducedGroup->findChildren<QWidget*>("", Qt::FindDirectChildrenOnly));

    ui->manufacturerLabel->setText("");
    ui->modelLabel->setText("");
    ui->swVersionLabel->setText("");
    ui->hwVersionLabel->setText("");
}

void LCCNodeInformation::newNodeFound(uint64_t node_id){
    if(node_id != m_nodeId) return;

    updateAllValuesForNode();
}

void LCCNodeInformation::updateAllValuesForNode(){
    char id_buffer[20];
    struct lcc_node_info* node_info = m_connection->lccNodeInfoForID(m_nodeId);
    enum lcc_protocols* protocols = NULL;
    int protocols_len = 0;

    if(node_info == nullptr){
        return;
    }

    lcc_node_id_to_dotted_format(m_nodeId, id_buffer, sizeof(id_buffer));
    ui->id_label->setText(QString(id_buffer));
    ui->alias_label->setText(QString::number(lcc_node_info_get_alias(node_info), 16));
    lcc_node_info_get_protocols_supported(node_info, &protocols, &protocols_len);
    for(int x = 0; x < protocols_len; x++){
        switch(protocols[x]){
        case LCC_PROTOCOL_INVALID:
            break;
        case LCC_PROTOCOL_SIMPLE:
            ui->simpleProtocol->setCheckState(Qt::Checked);
            break;
        case LCC_PROTOCOL_DATAGRAM:
            ui->datagram->setCheckState(Qt::Checked);
            break;
        case LCC_PROTOCOL_STREAM:
            ui->stream->setCheckState(Qt::Checked);
            break;
        case LCC_PROTOCOL_MEMORY_CONFIGURATION:
            ui->memoryConfig->setCheckState(Qt::Checked);
            break;
        case LCC_PROTOCOL_RESERVATION:
            ui->reservation->setCheckState(Qt::Checked);
            break;
        case LCC_PROTOCOL_PRODUCER_CONSUMER:
            ui->producerConsumer->setCheckState(Qt::Checked);
            break;
        case LCC_PROTOCOL_IDENTIFICATION:
            ui->identification->setCheckState(Qt::Checked);
            break;
        case LCC_PROTOCOL_TEACHING_LEARNING:
            ui->teachingLearning->setCheckState(Qt::Checked);
            break;
        case LCC_PROTOCOL_REMOTE_BUTTON:
            ui->remoteButton->setCheckState(Qt::Checked);
            break;
        case LCC_PROTOCOL_ABBREVIATED_DEFAULT_CDI:
            ui->abbreviatedCDI->setCheckState(Qt::Checked);
            break;
        case LCC_PROTOCOL_DISPLAY:
            ui->display->setCheckState(Qt::Checked);
            break;
        case LCC_PROTOCOL_SIMPLE_NODE_INFORMATION:
            ui->simpleNodeInfo->setCheckState(Qt::Checked);
            break;
        case LCC_PROTOCOL_CONFIGURATION_DESCRIPTION_INFORMATION:
            ui->CDI->setCheckState(Qt::Checked);
            break;
        case LCC_PROTOCOL_TRACTION_CONTROL:
            ui->tractionControl->setCheckState(Qt::Checked);
            break;
        case LCC_PROTOCOL_FUNCTION_DESCRIPTION_INFORMATION:
            ui->functionDescription->setCheckState(Qt::Checked);
            break;
        case LCC_PROTOCOL_DCC_COMMAND_STATION:
            ui->commandStation->setCheckState(Qt::Checked);
            break;
        case LCC_PROTOCOL_SIMPLE_TRAIN_NODE:
            ui->simpleTrainNode->setCheckState(Qt::Checked);
            break;
        case LCC_PROTOCOL_FUNCTION_CONFIGURATION:
            ui->functionConfiguration->setCheckState(Qt::Checked);
            break;
        case LCC_PROTOCOL_FIRMWARE_UPGRADE:
            ui->firmwareUpgrade->setCheckState(Qt::Checked);
            break;
        case LCC_PROTOCOL_FIRMWARE_UPGRADE_ACTIVE:
            ui->firmwareUpgradeActive->setCheckState(Qt::Checked);
            break;
        }
    }

    struct lcc_simple_node_info* simple = lcc_node_info_get_simple(node_info);
    ui->manufacturerLabel->setText(QString(simple->manufacturer_name));
    ui->modelLabel->setText(QString(simple->model_name));
    ui->swVersionLabel->setText(QString(simple->sw_version));
    ui->hwVersionLabel->setText(QString(simple->hw_version));
}

void LCCNodeInformation::on_queryProtocolsSimpleNode_clicked()
{
    struct lcc_node_info* node = m_connection->lccNodeInfoForID(m_nodeId);
    if(!node){
        return;
    }

    lcc_node_refresh_simple_info(node);
    lcc_node_refresh_protocol_support(node);
}


void LCCNodeInformation::on_queryEventsProducedConsumed_clicked()
{
    struct lcc_node_info* node = m_connection->lccNodeInfoForID(m_nodeId);
    if(!node){
        return;
    }

    lcc_node_refresh_events_produced(node);
    lcc_node_refresh_events_consumed(node);
}

void LCCNodeInformation::nodeUpdated(uint64_t node_id){
    if(m_nodeId != node_id){
        return;
    }

    updateAllValuesForNode();
}


void LCCNodeInformation::on_readCDI_clicked()
{
    std::shared_ptr<LCCNode> lccNode = m_connection->lccNodeForID(m_nodeId);

    lccNode->readCDI();
    connect(lccNode.get(), &LCCNode::cdiRead,
            [lccNode](){
        LOG4CXX_DEBUG_FMT(logger, "Raw CDI for node: {}", lccNode->rawCDI().toStdString());
        LOG4CXX_DEBUG_FMT(logger, "manufactuter: {} model: {} hardwareversion: {} SW version: {}",
                          lccNode->cdi().identification().manufacturer().toStdString(),
                          lccNode->cdi().identification().model().toStdString(),
                          lccNode->cdi().identification().hardwareVersion().toStdString(),
                          lccNode->cdi().identification().softwareVersion().toStdString());
    });
}
