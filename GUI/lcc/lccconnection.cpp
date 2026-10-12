/* SPDX-License-Identifier: GPL-2.0 */
#include "lccconnection.h"
#include "lcc-node-info.h"
#include "lcc-memory.h"
#include "lcc-datagram.h"
#include "lcc-event.h"
#include "lcc-remote-memory.h"
#include "lccnode.h"

#include <QTimer>

#include <log4cxx/logger.h>
#include <fmt/format.h>

static log4cxx::LoggerPtr logger = log4cxx::Logger::getLogger( "traingui.lcc.LCCConnection" );

static void new_node_cb(struct lcc_network_info* inf, struct lcc_node_info* new_node){
    LCCConnection* conn = static_cast<LCCConnection*>(lcc_network_get_userdata(inf));

    Q_EMIT conn->newNodeDiscovered(lcc_node_info_get_id(new_node));
}

static void node_updated_cb(struct lcc_network_info* inf, struct lcc_node_info* new_node){
    LCCConnection* conn = static_cast<LCCConnection*>(lcc_network_get_userdata(inf));

    Q_EMIT conn->nodeInformationUpdated(lcc_node_info_get_id(new_node));
}

static void incoming_event_cb(struct lcc_context* ctx, uint16_t source_alias, uint64_t event_id){
    LCCConnection* conn = static_cast<LCCConnection*>(lcc_context_user_data(ctx));

    Q_EMIT conn->incomingEvent(source_alias, event_id);
}

static void incoming_datagram_cb(struct lcc_datagram_context* ctx, uint16_t source_alias, void* datagram_data, int len){
    QByteArray ba = QByteArray::fromRawData((const char*)datagram_data, len);
    lcc_context* pctx = lcc_datagram_context_parent(ctx);

    LCCConnection* conn = static_cast<LCCConnection*>(lcc_context_user_data(pctx));

    LOG4CXX_DEBUG_FMT(logger, "Incoming datagram of len {}", len);

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

static void memory_space_info_query_cb(struct lcc_memory_context* ctx, uint16_t alias, uint8_t address_space){
    LOG4CXX_DEBUG_FMT(logger, "Query info for memory space 0x{:X}", address_space);
}

static void memory_space_read(struct lcc_memory_context* ctx, uint16_t alias, uint8_t address_space, uint32_t starting_address, uint8_t read_count){
    LOG4CXX_DEBUG_FMT(logger, "Read space 0x{:X} starting at {}", address_space, starting_address);

    lcc_memory_respond_read_reply_fail(ctx, alias, address_space, starting_address, 0, nullptr);
}

static void memory_space_write(struct lcc_memory_context* ctx, uint16_t alias, uint8_t address_space, uint32_t starting_address, void* data, int data_len){
    LOG4CXX_DEBUG_FMT(logger, "Write space 0x{:X} starting at {} length of {}", address_space, starting_address, data_len);

    lcc_memory_respond_write_reply_fail(ctx, alias, address_space, starting_address, 0, nullptr);
}

// How long to wait for a node to respond to a memory request
static constexpr int MEMORY_REQUEST_TIMEOUT_MS = 5000;
// Longest time that we will wait when a node says that its reply will take a while
static constexpr int MEMORY_REQUEST_MAX_TIMEOUT_MS = 64000;
// How many times to send a request again if the node doesn't respond, or is busy
static constexpr int MEMORY_REQUEST_MAX_RETRIES = 2;
static constexpr int MEMORY_REQUEST_RETRY_DELAY_MS = 250;

void LCCConnection::remote_memory_request_okCB(struct lcc_remote_memory_context* ctx, uint16_t alias, uint8_t flags){
    LOG4CXX_DEBUG_FMT(logger, "Remote memory request OK, flags 0x{:02X}", flags);

    lcc_context* pctx = lcc_remote_memory_parent(ctx);
    LCCConnection* conn = static_cast<LCCConnection*>(lcc_context_user_data(pctx));

    conn->remoteMemoryRequestAcknowledged(alias, flags);
}

void LCCConnection::remote_memory_request_failCB(struct lcc_remote_memory_context* ctx, uint16_t alias, uint16_t error_code, void* optional_data, int optional_len){
    LOG4CXX_DEBUG_FMT(logger, "Remote memory request to alias {:X} failed: error code 0x{:04X}", alias, error_code);

    lcc_context* pctx = lcc_remote_memory_parent(ctx);
    LCCConnection* conn = static_cast<LCCConnection*>(lcc_context_user_data(pctx));

    conn->remoteMemoryRequestFailed(alias, error_code);
}

void LCCConnection::remote_memory_receivedCB(struct lcc_remote_memory_context* ctx, uint16_t alias, uint8_t address_space, uint32_t starting_address, void* memory_data, int len){
    LOG4CXX_DEBUG_FMT(logger, "Remote memory recveived for alias {:X} space {:X} length {}", alias, address_space, len);

    lcc_context* pctx = lcc_remote_memory_parent(ctx);
    LCCConnection* conn = static_cast<LCCConnection*>(lcc_context_user_data(pctx));

    conn->remoteMemoryReceived(alias, address_space, starting_address, memory_data, len);
}

void LCCConnection::remote_memory_read_rejectCB(struct lcc_remote_memory_context* ctx, uint16_t alias, uint8_t address_space, uint32_t starting_address, uint16_t error_code, const char* message){
    LOG4CXX_DEBUG_FMT(logger, "Remote memory read of space {:X} address {} from alias {:X} rejected: error code 0x{:04X}",
                      address_space, starting_address, alias, error_code);

    lcc_context* pctx = lcc_remote_memory_parent(ctx);
    LCCConnection* conn = static_cast<LCCConnection*>(lcc_context_user_data(pctx));

    conn->remoteMemoryReadRejected(alias, address_space, starting_address, error_code, message);
}

void LCCConnection::remote_memory_write_okCB(struct lcc_remote_memory_context* ctx, uint16_t alias, uint8_t address_space, uint32_t starting_address){
    LOG4CXX_DEBUG_FMT(logger, "Remote memory write of space {:X} address {} to alias {:X} OK",
                      address_space, starting_address, alias);

    lcc_context* pctx = lcc_remote_memory_parent(ctx);
    LCCConnection* conn = static_cast<LCCConnection*>(lcc_context_user_data(pctx));

    conn->remoteMemoryWriteFinished(alias, address_space, true, 0, QString());
}

void LCCConnection::remote_memory_write_rejectedCB(struct lcc_remote_memory_context* ctx, uint16_t alias, uint8_t address_space, uint32_t starting_address, uint16_t error_code, const char* message){
    LOG4CXX_DEBUG_FMT(logger, "Remote memory write of space {:X} address {} to alias {:X} rejected: error code 0x{:04X}",
                      address_space, starting_address, alias, error_code);

    lcc_context* pctx = lcc_remote_memory_parent(ctx);
    LCCConnection* conn = static_cast<LCCConnection*>(lcc_context_user_data(pctx));

    QString fullMessage = QString("Write of address %1 rejected by node: %2")
            .arg(starting_address)
            .arg(errorCodeDescription(error_code));
    if(message && message[0] != 0){
        fullMessage += QString(" (%1)").arg(QString::fromUtf8(message));
    }

    conn->remoteMemoryWriteFinished(alias, address_space, false, error_code, fullMessage);
}

void LCCConnection::remote_memory_update_complete_okCB(struct lcc_remote_memory_context* ctx, uint16_t alias){
    LOG4CXX_DEBUG_FMT(logger, "Update complete accepted by alias {:X}", alias);

    lcc_context* pctx = lcc_remote_memory_parent(ctx);
    LCCConnection* conn = static_cast<LCCConnection*>(lcc_context_user_data(pctx));

    conn->remoteMemoryUpdateCompleteFinished(alias);
}

void LCCConnection::remote_memory_informationCB(struct lcc_remote_memory_context* ctx, uint16_t alias, int exists, int readonly, uint8_t address_space, uint32_t lowest_address, uint32_t highest_address, const char* message){
    LOG4CXX_DEBUG_FMT(logger, "Remote memory information for space {} rx", address_space);

    lcc_context* pctx = lcc_remote_memory_parent(ctx);
    LCCConnection* conn = static_cast<LCCConnection*>(lcc_context_user_data(pctx));

    conn->remoteMemoryInformation(alias, exists, readonly, address_space, lowest_address, highest_address, message);
}

LCCConnection::LCCConnection(QObject *parent) : SystemConnection(parent)
{
    m_requestTimer = new QTimer(this);
    m_requestTimer->setSingleShot(true);
    connect(m_requestTimer, &QTimer::timeout, this, &LCCConnection::memoryRequestTimedOut);

    m_lcc = lcc_context_new();
    m_lccNetwork = lcc_network_new(m_lcc);
    struct lcc_datagram_context* datagram_ctx = lcc_datagram_context_new(m_lcc);
    struct lcc_memory_context* memory_ctx = lcc_memory_new(m_lcc);
    struct lcc_remote_memory_context* remote_ctx = lcc_remote_memory_new(m_lcc);
    struct lcc_event_context* event_ctx = lcc_event_new(m_lcc);
    // We want to know about every event on the network, not just ones that we consume
    lcc_event_set_incoming_event_function(event_ctx, incoming_event_cb);
    lcc_event_set_listen_all_events(event_ctx, 1);

    lcc_context_set_userdata(m_lcc, this);
    lcc_datagram_context_set_datagram_functions(datagram_ctx,
                                       incoming_datagram_cb,
                                       datagram_recevied_ok,
                                       datagram_rejected);


    lcc_memory_set_memory_functions(memory_ctx,
                                    memory_space_info_query_cb,
                                    memory_space_read,
                                    memory_space_write);

    lcc_remote_memory_set_functions(remote_ctx,
                                    remote_memory_request_okCB,
                                    remote_memory_request_failCB,
                                    remote_memory_receivedCB,
                                    remote_memory_read_rejectCB,
                                    remote_memory_informationCB);
    lcc_remote_memory_set_write_functions(remote_ctx,
                                          remote_memory_write_okCB,
                                          remote_memory_write_rejectedCB);
    lcc_remote_memory_set_update_complete_function(remote_ctx, remote_memory_update_complete_okCB);

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
    lcc_event_context* ctx = lcc_context_get_event_context(m_lcc);
    lcc_event_produce_event(ctx, event_id);
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

QVector<uint64_t> LCCConnection::nodeIds(){
    struct lcc_node_info* node_info_list[120];
    QVector<uint64_t> ids;

    int num_nodes = lcc_network_get_node_list(m_lccNetwork, node_info_list, 120);
    for(int x = 0; x < num_nodes; x++){
        ids.push_back(lcc_node_info_get_id(node_info_list[x]));
    }

    return ids;
}

uint64_t LCCConnection::nodeIdForAlias(uint16_t alias){
    struct lcc_node_info* node_info_list[120];

    int num_nodes = lcc_network_get_node_list(m_lccNetwork, node_info_list, 120);
    for(int x = 0; x < num_nodes; x++){
        if(lcc_node_info_get_alias(node_info_list[x]) == alias){
            return lcc_node_info_get_id(node_info_list[x]);
        }
    }

    return 0;
}

AddressSpaceReadReply* LCCConnection::readSingleMemoryBlock(int alias, int space, uint32_t starting_address, int len){
    if(alias < 0 || space < 0 || space > 255 || len > 64){
        LOG4CXX_ERROR_FMT(logger, "Alias or space invalid.  alias: {} space: {}", alias, space);
        return nullptr;
    }

    AddressSpaceReadReply* rep = new AddressSpaceReadReply(alias, space, starting_address, len, this);

    m_inflight_read_replies.push_back(rep);
    m_request_order.push_back(rep);

    if(!m_requesting_memory){
        requestNextMemoryBlock();
    }

    return rep;
}

AddressSpaceWriteReply* LCCConnection::writeSingleMemoryBlock(int alias, int space, uint32_t starting_address, const QVector<uint8_t>& data){
    if(alias < 0 || space < 0 || space > 255 || data.isEmpty() || data.size() > 64){
        LOG4CXX_ERROR_FMT(logger, "Invalid write.  alias: {} space: {} length: {}", alias, space, data.size());
        return nullptr;
    }

    AddressSpaceWriteReply* rep = new AddressSpaceWriteReply(alias, space, starting_address, data, this);

    m_inflight_write_replies.push_back(rep);
    m_request_order.push_back(rep);

    if(!m_requesting_memory){
        requestNextMemoryBlock();
    }

    return rep;
}

UpdateCompleteReply* LCCConnection::sendUpdateComplete(int alias){
    if(alias <= 0){
        return nullptr;
    }

    UpdateCompleteReply* rep = new UpdateCompleteReply(alias, this);

    m_inflight_update_replies.push_back(rep);
    m_request_order.push_back(rep);

    if(!m_requesting_memory){
        requestNextMemoryBlock();
    }

    return rep;
}

std::shared_ptr<LCCNode> LCCConnection::lccNodeForID(uint64_t node_id){
    if(m_nodes.contains(node_id)){
        return m_nodes[node_id];
    }

    std::shared_ptr<LCCNode> lccNode = std::make_shared<LCCNode>(lccNodeInfoForID(node_id), this);
    if(lccNode->valid()){
        // If the node is valid(e.g. it exists in the C world), then we store the pointer.
        // Otherwise we don't, as we assume that this is an invalid node
        m_nodes[node_id] = lccNode;
    }

    return lccNode;
}

void LCCConnection::setCDI(QString cdi){
    m_cdi = cdi.toUtf8();
    lcc_memory_context* ctx = lcc_context_get_memory_context(m_lcc);
    lcc_memory_set_cdi(ctx, m_cdi.data(), m_cdi.length(), 0);
}

QString LCCConnection::connectionType(){
    return "lcc";
}

std::shared_ptr<Turnout> LCCConnection::getDCCTurnout(int switch_num){
    return nullptr;
}

AddressSpaceReply* LCCConnection::queryAddressSpaceInformation(int alias, int space){
    if(alias < 0 || space < 0 || space > 255){
        LOG4CXX_ERROR_FMT(logger, "Alias or space invalid.  alias: {} space: {}", alias, space);
        return nullptr;
    }

    AddressSpaceReply* rep = new AddressSpaceReply(alias, space, this);

    m_inflight_replies.push_back(rep);
    m_request_order.push_back(rep);

    if(!m_requesting_memory){
        requestNextMemoryBlock();
    }

    return rep;
}

void LCCConnection::remoteMemoryInformation(uint16_t alias, int exists, int readonly, uint8_t address_space, uint32_t lowest_address, uint32_t highest_address, const char* message){
    AddressSpaceReply* finished = nullptr;

    for(AddressSpaceReply* rep : m_inflight_replies){
        if(rep->isFinished()){
            continue;
        }

        if(rep->alias() == alias && rep->space() == address_space){
            rep->setValidResponse(!!exists, !!readonly, lowest_address, highest_address, message);
            finished = rep;
            break;
        }
    }

    if(finished){
        LOG4CXX_DEBUG_FMT(logger, "Got remote memory information for alias {:X}", alias);
        Q_EMIT addressSpaceRequestFinished(finished);
        m_inflight_replies.removeOne(finished);
    }

    memoryRequestDone();
}

void LCCConnection::remoteMemoryReceived(uint16_t alias, uint8_t address_space, uint32_t starting_address, void* memory_data, int len){
    AddressSpaceReadReply* finished = nullptr;

    for(AddressSpaceReadReply* rep : m_inflight_read_replies){
        if(rep->isFinished()){
            continue;
        }

        if(rep->alias() == alias && rep->space() == address_space){
            rep->setValidResponse(starting_address, memory_data, len);
            finished = rep;
            break;
        }
    }

    if(finished){
        Q_EMIT addressSpaceReadRequestFinished(finished);
        m_inflight_read_replies.removeOne(finished);
    }

    memoryRequestDone();
}

void LCCConnection::remoteMemoryRequestFailed(uint16_t alias, uint16_t error_code){
    // Only one request is outstanding at a time, so the rejection is for the current one
    if(!m_requesting_memory || m_request_order.isEmpty() || requestAlias(m_request_order.front()) != alias){
        return;
    }

    if((error_code & LCC_ERRCODE_TEMPORARY) && m_requestRetries < MEMORY_REQUEST_MAX_RETRIES){
        // The node is busy and says to try again
        LOG4CXX_DEBUG_FMT(logger, "Node {:X} temporarily rejected request(0x{:04X}), retrying", alias, error_code);
        retryCurrentRequest();
        return;
    }

    failCurrentRequest(error_code, QString("Request rejected by node: %1").arg(errorCodeDescription(error_code)));
}

void LCCConnection::remoteMemoryReadRejected(uint16_t alias, uint8_t address_space, uint32_t starting_address, uint16_t error_code, const char* message){
    AddressSpaceReadReply* failed = nullptr;

    for(AddressSpaceReadReply* rep : m_inflight_read_replies){
        if(rep->isFinished()){
            continue;
        }

        if(rep->alias() == alias && rep->space() == address_space){
            failed = rep;
            break;
        }
    }

    if(!failed){
        return;
    }

    QString fullMessage = QString("Read of address %1 rejected by node: %2")
            .arg(starting_address)
            .arg(errorCodeDescription(error_code));
    if(message && message[0] != 0){
        fullMessage += QString(" (%1)").arg(QString::fromUtf8(message));
    }
    QByteArray fullMessageUtf8 = fullMessage.toUtf8();

    m_inflight_read_replies.removeOne(failed);
    failed->setInvalidResponse(error_code, fullMessageUtf8.constData());
    Q_EMIT addressSpaceReadRequestFinished(failed);

    memoryRequestDone();
}

QString LCCConnection::errorCodeDescription(uint16_t error_code){
    QString description;

    switch(error_code){
    case LCC_ERRCODE_PERMANENT: description = "Permanent error"; break;
    case LCC_ERRCODE_SOURCE_NOT_PERMITTED: description = "Source not permitted"; break;
    case LCC_ERRCODE_NOT_IMPLEMENTED: description = "Not implemented"; break;
    case LCC_ERRCODE_NOT_IMPLEMENTED_SUBCOMMAND: description = "Not implemented: subcommand unknown"; break;
    case LCC_ERRCODE_NOT_IMPLEMENTED_DATAGRAM_STREAM: description = "Not implemented: datagram/stream type unknown"; break;
    case LCC_ERRCODE_NOT_IMPLEMENTED_UNKNOWN_MTI: description = "Not implemented: unknown MTI or transport protocol"; break;
    case LCC_ERRCODE_INVALID_ARG: description = "Invalid arguments"; break;
    case LCC_ERRCODE_TEMPORARY: description = "Temporary error"; break;
    case LCC_ERRCODE_TIMEOUT: description = "Timeout"; break;
    case LCC_ERRCODE_TIMEOUT_END_OF_FRAME: description = "Timeout waiting for end frame"; break;
    case LCC_ERRCODE_BUFFER_UNAVAILABLE: description = "Buffer unavailable"; break;
    case LCC_ERRCODE_NOT_EXPECTED: description = "Not expected: out of order"; break;
    case LCC_ERRCODE_OUT_OF_ORDER_MIDDLE_END: description = "Out of order: middle or end frame without start"; break;
    case LCC_ERRCODE_OUT_OF_ORDER_START_BEFORE_FINISH: description = "Out of order: start frame before previous finished"; break;
    case LCC_ERRCODE_TRANSFER_ERROR: description = "Transfer error"; break;
    default:
        if(error_code & 0x1000){
            description = "Permanent error";
        }else if(error_code & 0x2000){
            description = "Temporary error";
        }else{
            description = "Unknown error";
        }
        break;
    }

    return QString("%1 (0x%2)").arg(description).arg(error_code, 4, 16, QLatin1Char('0'));
}

void LCCConnection::remoteMemoryWriteFinished(uint16_t alias, uint8_t address_space, bool ok, uint16_t error_code, const QString& message){
    AddressSpaceWriteReply* finished = nullptr;

    for(AddressSpaceWriteReply* rep : m_inflight_write_replies){
        if(rep->isFinished()){
            continue;
        }

        if(rep->alias() == alias && rep->space() == address_space){
            finished = rep;
            break;
        }
    }

    if(finished){
        m_inflight_write_replies.removeOne(finished);
        if(ok){
            finished->setValidResponse();
        }else{
            finished->setInvalidResponse(error_code, message);
        }
    }

    memoryRequestDone();
}

void LCCConnection::remoteMemoryUpdateCompleteFinished(uint16_t alias){
    UpdateCompleteReply* finished = nullptr;
    for(UpdateCompleteReply* rep : m_inflight_update_replies){
        if(!rep->isFinished() && rep->alias() == alias){
            finished = rep;
            break;
        }
    }

    if(finished){
        m_inflight_update_replies.removeOne(finished);
        finished->setValidResponse();
    }

    memoryRequestDone();
}

void LCCConnection::memoryRequestDone(){
    m_requestTimer->stop();
    m_requestRetries = 0;
    m_requesting_memory = false;
    QTimer::singleShot(20, this, &LCCConnection::requestNextMemoryBlock);
}

void LCCConnection::remoteMemoryRequestAcknowledged(uint16_t alias, uint8_t flags){
    if(!m_requesting_memory){
        return;
    }

    // The node has our request.  If a reply is coming, the node may tell us how long
    // it could take(2^N seconds), otherwise use our normal timeout from now.
    int timeoutMs = MEMORY_REQUEST_TIMEOUT_MS;
    int exponent = flags & 0x0F;
    if((flags & LCC_DATAGRAM_REPLY_PENDING) && exponent > 0){
        timeoutMs = std::min((1 << exponent) * 1000, MEMORY_REQUEST_MAX_TIMEOUT_MS);
        timeoutMs = std::max(timeoutMs, MEMORY_REQUEST_TIMEOUT_MS);
    }
    m_requestTimer->start(timeoutMs);
}

void LCCConnection::memoryRequestTimedOut(){
    if(!m_requesting_memory || m_request_order.isEmpty()){
        return;
    }

    // Let the library send something else
    lcc_remote_memory_context* ctx = lcc_context_get_remote_memory_context(m_lcc);
    lcc_remote_memory_cancel_request(ctx);

    QObject* current = m_request_order.front();
    if(m_requestRetries < MEMORY_REQUEST_MAX_RETRIES){
        // A frame may have been lost, so try again before giving up
        LOG4CXX_WARN_FMT(logger, "No response from node {:X}, retrying", requestAlias(current));
        retryCurrentRequest();
        return;
    }

    QString message;
    if(auto rep = qobject_cast<AddressSpaceReadReply*>(current)){
        message = QString("Node 0x%1 did not respond to a read of space 0x%2 address %3")
                .arg(rep->alias(), 3, 16, QLatin1Char('0'))
                .arg(rep->space(), 2, 16, QLatin1Char('0'))
                .arg(rep->startingAddress());
    }else if(auto rep = qobject_cast<AddressSpaceReply*>(current)){
        message = QString("Node 0x%1 did not respond to a request for information about space 0x%2")
                .arg(rep->alias(), 3, 16, QLatin1Char('0'))
                .arg(rep->space(), 2, 16, QLatin1Char('0'));
    }else if(auto rep = qobject_cast<AddressSpaceWriteReply*>(current)){
        message = QString("Node 0x%1 did not respond to a write of space 0x%2 address %3")
                .arg(rep->alias(), 3, 16, QLatin1Char('0'))
                .arg(rep->space(), 2, 16, QLatin1Char('0'))
                .arg(rep->startingAddress());
    }else if(auto rep = qobject_cast<UpdateCompleteReply*>(current)){
        message = QString("Node 0x%1 did not respond to 'update complete'")
                .arg(rep->alias(), 3, 16, QLatin1Char('0'));
    }
    LOG4CXX_ERROR_FMT(logger, "{}", message.toStdString());

    failCurrentRequest(0, message);
}

uint16_t LCCConnection::requestAlias(QObject* request){
    if(auto rep = qobject_cast<AddressSpaceReadReply*>(request)){
        return rep->alias();
    }else if(auto rep = qobject_cast<AddressSpaceReply*>(request)){
        return rep->alias();
    }else if(auto rep = qobject_cast<AddressSpaceWriteReply*>(request)){
        return rep->alias();
    }else if(auto rep = qobject_cast<UpdateCompleteReply*>(request)){
        return rep->alias();
    }
    return 0;
}

void LCCConnection::retryCurrentRequest(){
    m_requestRetries++;
    m_requestTimer->stop();
    m_requesting_memory = false;
    // The current request is still at the front of the queue, so it is sent again
    QTimer::singleShot(MEMORY_REQUEST_RETRY_DELAY_MS, this, &LCCConnection::requestNextMemoryBlock);
}

void LCCConnection::failCurrentRequest(uint16_t error_code, const QString& message){
    QObject* current = m_request_order.front();
    m_request_order.pop_front();
    QByteArray messageUtf8 = message.toUtf8();

    if(auto rep = qobject_cast<AddressSpaceReadReply*>(current)){
        m_inflight_read_replies.removeOne(rep);
        rep->setInvalidResponse(error_code, messageUtf8.constData());
        Q_EMIT addressSpaceReadRequestFinished(rep);
    }else if(auto rep = qobject_cast<AddressSpaceReply*>(current)){
        m_inflight_replies.removeOne(rep);
        rep->setInvalidResponse(0, error_code, messageUtf8.constData());
        Q_EMIT addressSpaceRequestFinished(rep);
    }else if(auto rep = qobject_cast<AddressSpaceWriteReply*>(current)){
        m_inflight_write_replies.removeOne(rep);
        rep->setInvalidResponse(error_code, message);
    }else if(auto rep = qobject_cast<UpdateCompleteReply*>(current)){
        m_inflight_update_replies.removeOne(rep);
        rep->setInvalidResponse(error_code, message);
    }

    memoryRequestDone();
}

void LCCConnection::requestNextMemoryBlock(){
    if(m_requesting_memory){
        LOG4CXX_ERROR_FMT(logger, "Can't request next memory block: operation still in progress");
        return;
    }

    // Requests are sent in the order that they were made.  Skip over anything that
    // has already finished(e.g. failed because the node rejected an earlier datagram)
    while(!m_request_order.isEmpty()){
        QObject* front = m_request_order.front();
        bool done = front == nullptr;
        if(auto rep = qobject_cast<AddressSpaceReadReply*>(front)){
            done = rep->isFinished();
        }else if(auto rep = qobject_cast<AddressSpaceReply*>(front)){
            done = rep->isFinished();
        }else if(auto rep = qobject_cast<AddressSpaceWriteReply*>(front)){
            done = rep->isFinished();
        }else if(auto rep = qobject_cast<UpdateCompleteReply*>(front)){
            done = rep->isFinished();
        }

        if(!done){
            break;
        }
        m_request_order.pop_front();
    }

    if(m_request_order.isEmpty()){
        return;
    }

    m_requesting_memory = true;

    lcc_remote_memory_context* ctx = lcc_context_get_remote_memory_context(m_lcc);
    QObject* next = m_request_order.front();
    int ret = LCC_OK;

    if(auto rep = qobject_cast<AddressSpaceReadReply*>(next)){
        ret = lcc_remote_memory_read_single_transfer(ctx, rep->alias(), rep->space(), rep->startingAddress(), rep->requestedLen());
        if(ret != LCC_OK){
            LOG4CXX_ERROR_FMT(logger, "Unable to send memory read request to alias {:X}: error {}", rep->alias(), ret);
            m_inflight_read_replies.removeOne(rep);
            rep->setInvalidResponse(0, QString("Unable to send read request(LCC error %1)").arg(ret).toUtf8().constData());
        }
    }else if(auto rep = qobject_cast<AddressSpaceReply*>(next)){
        ret = lcc_remote_memory_get_address_space_information(ctx, rep->alias(), rep->space());
        if(ret != LCC_OK){
            LOG4CXX_ERROR_FMT(logger, "Unable to send address space information request to alias {:X}: error {}", rep->alias(), ret);
            m_inflight_replies.removeOne(rep);
            rep->setInvalidResponse(0, 0, QString("Unable to send address space request(LCC error %1)").arg(ret).toUtf8().constData());
        }
    }else if(auto rep = qobject_cast<AddressSpaceWriteReply*>(next)){
        QVector<uint8_t> data = rep->data();
        ret = lcc_remote_memory_write_single_transfer(ctx, rep->alias(), rep->space(), rep->startingAddress(), data.constData(), data.size());
        if(ret != LCC_OK){
            LOG4CXX_ERROR_FMT(logger, "Unable to send memory write request to alias {:X}: error {}", rep->alias(), ret);
            m_inflight_write_replies.removeOne(rep);
            rep->setInvalidResponse(0, QString("Unable to send write request(LCC error %1)").arg(ret));
        }
    }else if(auto rep = qobject_cast<UpdateCompleteReply*>(next)){
        ret = lcc_remote_memory_update_complete(ctx, rep->alias());
        if(ret != LCC_OK){
            LOG4CXX_ERROR_FMT(logger, "Unable to send update complete to alias {:X}: error {}", rep->alias(), ret);
            m_inflight_update_replies.removeOne(rep);
            rep->setInvalidResponse(0, QString("Unable to send update complete(LCC error %1)").arg(ret));
        }
    }

    if(ret == LCC_OK){
        m_requestTimer->start(MEMORY_REQUEST_TIMEOUT_MS);
    }else{
        // Nothing was sent, so nothing will come back - move on to the next request
        m_request_order.pop_front();
        memoryRequestDone();
    }
}
