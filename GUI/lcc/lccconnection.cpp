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

static void remote_memory_request_ok(struct lcc_remote_memory_context* ctx, uint16_t alias, uint8_t flags){
    LOG4CXX_DEBUG_FMT(logger, "Remote memory request OK");
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

void LCCConnection::remote_memory_informationCB(struct lcc_remote_memory_context* ctx, uint16_t alias, int exists, int readonly, uint8_t address_space, uint32_t lowest_address, uint32_t highest_address, const char* message){
    LOG4CXX_DEBUG_FMT(logger, "Remote memory information for space {} rx", address_space);

    lcc_context* pctx = lcc_remote_memory_parent(ctx);
    LCCConnection* conn = static_cast<LCCConnection*>(lcc_context_user_data(pctx));

    conn->remoteMemoryInformation(alias, exists, readonly, address_space, lowest_address, highest_address, message);
}

LCCConnection::LCCConnection(QObject *parent) : SystemConnection(parent)
{
    m_lcc = lcc_context_new();
    m_lccNetwork = lcc_network_new(m_lcc);
    struct lcc_datagram_context* datagram_ctx = lcc_datagram_context_new(m_lcc);
    struct lcc_memory_context* memory_ctx = lcc_memory_new(m_lcc);
    struct lcc_remote_memory_context* remote_ctx = lcc_remote_memory_new(m_lcc);
    lcc_event_new(m_lcc);

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
                                    remote_memory_request_ok,
                                    remote_memory_request_failCB,
                                    remote_memory_receivedCB,
                                    remote_memory_read_rejectCB,
                                    remote_memory_informationCB);

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

AddressSpaceReadReply* LCCConnection::readSingleMemoryBlock(int alias, int space, uint32_t starting_address, int len){
    if(alias < 0 || space < 0 || space > 255 || len > 64){
        LOG4CXX_ERROR_FMT(logger, "Alias or space invalid.  alias: {} space: {}", alias, space);
        return nullptr;
    }

    AddressSpaceReadReply* rep = new AddressSpaceReadReply(alias, space, starting_address, len, this);

    m_inflight_read_replies.push_back(rep);

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

    m_requesting_memory = false;
    QTimer::singleShot(20, this, &LCCConnection::requestNextMemoryBlock);
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

    m_requesting_memory = false;
    QTimer::singleShot(20, this, &LCCConnection::requestNextMemoryBlock);
}

void LCCConnection::remoteMemoryRequestFailed(uint16_t alias, uint16_t error_code){
    // The datagram rejection doesn't tell us which request failed, but only one
    // memory request can be outstanding to a node at a time, so fail everything for this alias.
    QByteArray message = QString("Request rejected by node: %1")
            .arg(errorCodeDescription(error_code))
            .toUtf8();

    QVector<AddressSpaceReply*> failedInfo;
    for(AddressSpaceReply* rep : m_inflight_replies){
        if(!rep->isFinished() && rep->alias() == alias){
            failedInfo.push_back(rep);
        }
    }
    QVector<AddressSpaceReadReply*> failedReads;
    for(AddressSpaceReadReply* rep : m_inflight_read_replies){
        if(!rep->isFinished() && rep->alias() == alias){
            failedReads.push_back(rep);
        }
    }

    // Remove from the inflight lists before notifying, as the receivers may make new requests
    for(AddressSpaceReply* rep : failedInfo){
        m_inflight_replies.removeOne(rep);
    }
    for(AddressSpaceReadReply* rep : failedReads){
        m_inflight_read_replies.removeOne(rep);
    }

    for(AddressSpaceReply* rep : failedInfo){
        rep->setInvalidResponse(0, error_code, message.constData());
        Q_EMIT addressSpaceRequestFinished(rep);
    }
    for(AddressSpaceReadReply* rep : failedReads){
        rep->setInvalidResponse(error_code, message.constData());
        Q_EMIT addressSpaceReadRequestFinished(rep);
    }

    m_requesting_memory = false;
    QTimer::singleShot(20, this, &LCCConnection::requestNextMemoryBlock);
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

    m_requesting_memory = false;
    QTimer::singleShot(20, this, &LCCConnection::requestNextMemoryBlock);
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

void LCCConnection::requestNextMemoryBlock(){
    if(m_requesting_memory){
        LOG4CXX_ERROR_FMT(logger, "Can't requets next memory block: operation still in progress");
        return;
    }

    if(m_inflight_read_replies.empty() && m_inflight_replies.empty()){
        m_requesting_memory = false;
        return;
    }

    m_requesting_memory = true;

    lcc_remote_memory_context* ctx = lcc_context_get_remote_memory_context(m_lcc);

    if(!m_inflight_read_replies.empty()){
        AddressSpaceReadReply* rep = m_inflight_read_replies.front();
        int ret = lcc_remote_memory_read_single_transfer(ctx, rep->alias(), rep->space(), rep->startingAddress(), rep->requestedLen());
        if(ret != LCC_OK){
            LOG4CXX_ERROR_FMT(logger, "Unable to send memory read request to alias {:X}: error {}", rep->alias(), ret);
            m_inflight_read_replies.removeOne(rep);
            rep->setInvalidResponse(ret, "Internal LCC error");
        }
    }else if(!m_inflight_replies.empty()){
        AddressSpaceReply* rep = m_inflight_replies.front();
        int ret = lcc_remote_memory_get_address_space_information(ctx, rep->alias(), rep->space());
        if(ret != LCC_OK){
            LOG4CXX_ERROR_FMT(logger, "Unable to send address space information request to alias {:X}: error {}", rep->alias(), ret);
            m_inflight_replies.removeOne(rep);
            rep->setInvalidResponse(0, ret, "Internal LCC error");
        }
    }
}
