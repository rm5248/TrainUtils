/* SPDX-License-Identifier: GPL-2.0 */
#include "lccconnection.h"
#include "lcc-node-info.h"
#include "lcc-memory.h"
#include "lcc-datagram.h"
#include "lccnode.h"

static void new_node_cb(struct lcc_network_info* inf, struct lcc_node_info* new_node){
    LCCConnection* conn = static_cast<LCCConnection*>(lcc_network_get_userdata(inf));

    Q_EMIT conn->newNodeDiscovered(lcc_node_info_get_id(new_node));
}

static void node_updated_cb(struct lcc_network_info* inf, struct lcc_node_info* new_node){
    LCCConnection* conn = static_cast<LCCConnection*>(lcc_network_get_userdata(inf));

    Q_EMIT conn->nodeInformationUpdated(lcc_node_info_get_id(new_node));
}

static void incoming_datagram_cb(struct lcc_datagram_context* ctx, uint16_t source_alias, void* datagram_data, int len){
    QByteArray ba = QByteArray::fromRawData((const char*)datagram_data, len);
    lcc_context* pctx = lcc_datagram_context_parent(ctx);

    LCCConnection* conn = static_cast<LCCConnection*>(lcc_context_user_data(pctx));

    Q_EMIT conn->incomingDatagram(source_alias, ba);
}

static void datagram_recevied_ok(struct lcc_datagram_context* ctx, uint16_t source_alias, uint8_t flags){
    lcc_context* pctx = lcc_datagram_context_parent(ctx);
    LCCConnection* conn = static_cast<LCCConnection*>(lcc_context_user_data(pctx));

    Q_EMIT conn->datagramReceivedOK(source_alias, flags);
}

static void datagram_rejected(struct lcc_datagram_context* ctx, uint16_t source_alias, uint16_t error_code, void* optional_data, int optional_len){
    lcc_context* pctx = lcc_datagram_context_parent(ctx);
    LCCConnection* conn = static_cast<LCCConnection*>(lcc_context_user_data(pctx));
    QByteArray ba = QByteArray::fromRawData((const char*)optional_data, optional_len);

    Q_EMIT conn->datagramRejected(source_alias, error_code, ba);
}

LCCConnection::LCCConnection(QObject *parent) : SystemConnection(parent)
{
    m_lcc = lcc_context_new();
    m_lccNetwork = lcc_network_new(m_lcc);
    struct lcc_datagram_context* datagram_ctx = lcc_datagram_context_new(m_lcc);

    lcc_context_set_userdata(m_lcc, this);
    lcc_datagram_context_set_datagram_functions(datagram_ctx,
                                       incoming_datagram_cb,
                                       datagram_recevied_ok,
                                       datagram_rejected);

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
        return nullptr;
    }

    for( int x = 0; x < num_nodes; x++ ){
        uint64_t current_id = lcc_node_info_get_id(node_info_list[x]);
        if(node_id == current_id){
            return node_info_list[x];
        }
    }

    return nullptr;
}

void LCCConnection::readSingleMemoryBlock(int alias, int space, uint32_t starting_address, int len){
    lcc_memory_read_single_transfer(m_lcc, alias, space, starting_address, len);
}

std::shared_ptr<LCCNode> LCCConnection::lccNodeForID(uint64_t node_id){
    if(m_nodes.contains(node_id)){
        return m_nodes[node_id];
    }

    std::shared_ptr<LCCNode> lccNode = std::make_shared<LCCNode>(m_lcc, lccNodeInfoForID(node_id), this);
    if(lccNode->valid()){
        // If the node is valid(e.g. it exists in the C world), then we store the pointer.
        // Otherwise we don't, as we assume that this is an invalid node
        m_nodes[node_id] = lccNode;
    }

    return lccNode;
}
