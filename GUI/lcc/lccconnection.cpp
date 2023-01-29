/* SPDX-License-Identifier: GPL-2.0 */
#include "lccconnection.h"
#include "lcc-node-info.h"

static void new_node_cb(struct lcc_network_info* inf, struct lcc_node_info* new_node){
    LCCConnection* conn = static_cast<LCCConnection*>(lcc_network_get_userdata(inf));

    Q_EMIT conn->newNodeDiscovered(lcc_node_info_get_id(new_node));
}

static void node_updated_cb(struct lcc_network_info* inf, struct lcc_node_info* new_node){
    LCCConnection* conn = static_cast<LCCConnection*>(lcc_network_get_userdata(inf));

    Q_EMIT conn->nodeInformationUpdated(lcc_node_info_get_id(new_node));
}

LCCConnection::LCCConnection(QObject *parent) : SystemConnection(parent)
{
    m_lcc = lcc_context_new();
    m_lccNetwork = lcc_network_new(m_lcc);

    // TODO make this configurable.  currently set to 'assigned by software at runtime'
    lcc_context_set_unique_identifer(m_lcc, 0x040032405001);

    lcc_network_set_userdata(m_lccNetwork, this);
    lcc_network_set_new_node_callback(m_lccNetwork, new_node_cb);
    lcc_network_set_node_changed_callback(m_lccNetwork, node_updated_cb);
}

LCCConnection::~LCCConnection(){
    lcc_network_free(m_lccNetwork);
    lcc_context_free(m_lcc);
}

void LCCConnection::setSimpleNodeInformation(QString manufacturer,
                              QString model,
                              QString hwVersion,
                              QString swVersion){
    std::string manufStd = manufacturer.toStdString();
    std::string modelStd = model.toStdString();
    std::string hwStd = hwVersion.toStdString();
    std::string swStd = swVersion.toStdString();

    lcc_context_set_simple_node_information(m_lcc,
                                            manufStd.c_str(),
                                            modelStd.c_str(),
                                            hwStd.c_str(),
                                            swStd.c_str());
}

void LCCConnection::setSimpleNodeNameDescription(QString nodeName,
                                  QString nodeDescription){
    std::string nameStd = nodeName.toStdString();
    std::string descriptionStd = nodeDescription.toStdString();

    lcc_context_set_simple_node_name_description(m_lcc,
                                                 nameStd.c_str(),
                                                 descriptionStd.c_str());
}

void LCCConnection::sendEvent(uint64_t event_id){
    lcc_context_produce_event(m_lcc, event_id);
}

void LCCConnection::refreshNetwork(){
    lcc_network_refresh_nodes(m_lccNetwork);
}

struct lcc_node_info* LCCConnection::lccNodeInfoForID(uint64_t node_id){
    struct lcc_node_info* node_info_list[120];
    int num_nodes = 0;

    num_nodes = lcc_network_get_node_list(m_lccNetwork, node_info_list, 120);
    if(num_nodes < 0){
        return NULL;
    }

    for( int x = 0; x < num_nodes; x++ ){
        uint64_t current_id = lcc_node_info_get_id(node_info_list[x]);
        if(node_id == current_id){
            return node_info_list[x];
        }
    }

    return NULL;
}
