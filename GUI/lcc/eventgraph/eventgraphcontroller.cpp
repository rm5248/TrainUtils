/* SPDX-License-Identifier: GPL-2.0 */
#include "eventgraphcontroller.h"

#include <QTimer>

#include "lcc/lccconnection.h"
#include "lcc/lccnode.h"
#include "lcc/segmentmemory.h"
#include "lcc/updatecompletereply.h"
#include "lcc/cdi/cdilayout.h"
#include "lcc-node-info.h"
#include "lcc-simple-node-info.h"

#include <log4cxx/logger.h>
#include <fmt/format.h>

static log4cxx::LoggerPtr logger = log4cxx::Logger::getLogger( "traingui.lcc.EventGraphController" );

// How long to wait for a node to tell us what it is before reading its CDI anyway
static constexpr int NODE_INFO_TIMEOUT_MS = 3000;

// CDIs that have been read, by manufacturer/model/software version.  Nodes of the
// same model with the same software have the same CDI, so there's no need to read
// it from every one of them.  Shared by all graphs.
static QMap<QString, CDI> s_cdiCache;

EventGraphController::EventGraphController(std::shared_ptr<LCCConnection> conn, QObject* parent)
    : QObject(parent),
    m_conn(conn)
{
    connect(m_conn.get(), &LCCConnection::newNodeDiscovered,
            this, &EventGraphController::nodeDiscovered);
    connect(m_conn.get(), &LCCConnection::nodeInformationUpdated,
            this, &EventGraphController::nodeInformationUpdated);
    connect(m_conn.get(), &LCCConnection::incomingEvent,
            this, &EventGraphController::incomingEvent);
}

std::shared_ptr<LCCConnection> EventGraphController::connection() const{
    return m_conn;
}

EventGraphModel& EventGraphController::model(){
    return m_model;
}

NodeLoadStatus EventGraphController::loadStatus(uint64_t nodeId) const{
    return m_status.value(nodeId);
}

void EventGraphController::start(){
    for(uint64_t nodeId : m_conn->nodeIds()){
        nodeDiscovered(nodeId);
    }

    // Find anything that we don't know about yet
    m_conn->refreshNetwork();
}

void EventGraphController::reloadNode(uint64_t nodeId){
    std::shared_ptr<LCCNode> node = m_conn->lccNodeForID(nodeId);
    if(!node->valid()){
        return;
    }

    setStatus(nodeId, NodeLoadState::ReadingCdi);
    m_forceMemoryRead.insert(nodeId);
    // Always read the CDI from the node, in case the cached one is out of date
    readCdi(nodeId, true);
}

void EventGraphController::nodeDiscovered(uint64_t nodeId){
    if(m_status.contains(nodeId)){
        return;
    }

    EventGraphNode node;
    node.nodeId = nodeId;
    m_model.setNode(node);
    setStatus(nodeId, NodeLoadState::WaitingForInfo);
    updateNodeInformation(nodeId);

    struct lcc_node_info* info = m_conn->lccNodeInfoForID(nodeId);
    if(info){
        lcc_node_refresh_simple_info(info);
        lcc_node_refresh_protocol_support(info);
        lcc_node_refresh_events_produced(info);
        lcc_node_refresh_events_consumed(info);
    }

    if(!m_model.node(nodeId)->manufacturer.isEmpty()){
        // We already know what this node is
        readCdi(nodeId);
        return;
    }

    // Not all nodes respond, so don't wait forever
    QTimer::singleShot(NODE_INFO_TIMEOUT_MS, this, [this, nodeId](){
        if(m_status.value(nodeId).state == NodeLoadState::WaitingForInfo){
            readCdi(nodeId);
        }
    });
    Q_EMIT modelChanged();
}

void EventGraphController::nodeInformationUpdated(uint64_t nodeId){
    if(!m_status.contains(nodeId)){
        nodeDiscovered(nodeId);
        return;
    }

    updateNodeInformation(nodeId);

    if(m_status.value(nodeId).state == NodeLoadState::WaitingForInfo &&
            !m_model.node(nodeId)->manufacturer.isEmpty()){
        readCdi(nodeId);
    }
}

void EventGraphController::updateNodeInformation(uint64_t nodeId){
    struct lcc_node_info* info = m_conn->lccNodeInfoForID(nodeId);
    const EventGraphNode* current = m_model.node(nodeId);
    if(!info || !current){
        return;
    }

    struct lcc_simple_node_info* simple = lcc_node_info_get_simple(info);
    EventGraphNode updated = *current;
    updated.manufacturer = QString(lcc_simple_node_info_manufacturer_name(simple));
    updated.model = QString(lcc_simple_node_info_model_name(simple));
    updated.name = QString(lcc_simple_node_info_node_name(simple));
    if(updated.manufacturer != current->manufacturer ||
            updated.model != current->model ||
            updated.name != current->name){
        m_model.setNode(updated);
    }

    uint64_t* list = nullptr;
    int len = 0;
    QVector<uint64_t> produced;
    QVector<uint64_t> consumed;
    if(lcc_node_info_get_events_produced(info, &list, &len) == LCC_OK){
        for(int x = 0; x < len; x++){
            produced.push_back(list[x]);
        }
    }
    if(lcc_node_info_get_events_consumed(info, &list, &len) == LCC_OK){
        for(int x = 0; x < len; x++){
            consumed.push_back(list[x]);
        }
    }
    m_model.setIdentifiedEvents(nodeId, produced, consumed);

    Q_EMIT modelChanged();
}

QString EventGraphController::cdiCacheKey(uint64_t nodeId){
    struct lcc_node_info* info = m_conn->lccNodeInfoForID(nodeId);
    if(!info){
        return QString();
    }

    struct lcc_simple_node_info* simple = lcc_node_info_get_simple(info);
    QString manufacturer(lcc_simple_node_info_manufacturer_name(simple));
    QString model(lcc_simple_node_info_model_name(simple));
    if(manufacturer.isEmpty() || model.isEmpty()){
        // Not enough to know that two nodes are the same
        return QString();
    }

    return manufacturer + '\n' + model + '\n' + QString(lcc_simple_node_info_sw_version(simple));
}

void EventGraphController::readCdi(uint64_t nodeId, bool force){
    setStatus(nodeId, NodeLoadState::ReadingCdi);

    struct lcc_node_info* info = m_conn->lccNodeInfoForID(nodeId);
    if(info){
        enum lcc_protocols* protocols = nullptr;
        int protocolsLen = 0;
        lcc_node_info_get_protocols_supported(info, &protocols, &protocolsLen);
        bool hasCdi = false;
        for(int x = 0; x < protocolsLen; x++){
            if(protocols[x] == LCC_PROTOCOL_CONFIGURATION_DESCRIPTION_INFORMATION){
                hasCdi = true;
            }
        }
        if(protocolsLen > 0 && !hasCdi){
            // The node told us what it supports, and the CDI isn't one of them
            setStatus(nodeId, NodeLoadState::NoCdi);
            return;
        }
    }

    std::shared_ptr<LCCNode> node = m_conn->lccNodeForID(nodeId);
    if(!node->valid()){
        setStatus(nodeId, NodeLoadState::Failed, tr("Node not found"));
        return;
    }

    if(!m_connectedNodes.contains(nodeId)){
        m_connectedNodes.insert(nodeId);
        // Also called when the CDI is read by something else(e.g. a CDI editor)
        connect(node.get(), &LCCNode::cdiRead, this, [this, nodeId](){
            cdiAvailable(nodeId);
        });
        connect(node.get(), &LCCNode::cdiReadFailure, this, [this, nodeId](uint16_t, QString message){
            setStatus(nodeId, NodeLoadState::Failed, tr("Unable to read CDI: %1").arg(message));
        });
    }

    if(!force){
        if(node->hasCDI()){
            cdiAvailable(nodeId);
            return;
        }

        QString key = cdiCacheKey(nodeId);
        if(!key.isEmpty() && s_cdiCache.contains(key)){
            LOG4CXX_DEBUG_FMT(logger, "Using cached CDI for node {:X}", nodeId);
            node->setCDI(s_cdiCache.value(key));
            return;
        }
    }

    SegmentMemory* mem = node->readCDI();
    if(mem && !m_connectedMemories.contains(mem)){
        m_connectedMemories.insert(mem);
        connect(mem, &SegmentMemory::downloadProgress, this, [this, nodeId](uint64_t rx, uint64_t total){
            if(total > 0 && m_status.contains(nodeId)){
                m_status[nodeId].progress = static_cast<int>(rx * 100 / total);
            }
        });
    }
}

void EventGraphController::cdiAvailable(uint64_t nodeId){
    std::shared_ptr<LCCNode> node = m_conn->lccNodeForID(nodeId);
    const EventGraphNode* current = m_model.node(nodeId);
    if(!node->valid() || !current){
        return;
    }

    CDI cdi = node->cdi();
    QString key = cdiCacheKey(nodeId);
    if(!key.isEmpty()){
        s_cdiCache.insert(key, cdi);
    }

    EventGraphNode updated = *current;
    updated.pins = eventPinsFromLayout(cdiLayout(cdi));
    m_model.setNode(updated);
    Q_EMIT modelChanged();

    readEventIds(nodeId);
}

void EventGraphController::readEventIds(uint64_t nodeId){
    std::shared_ptr<LCCNode> node = m_conn->lccNodeForID(nodeId);
    const EventGraphNode* current = m_model.node(nodeId);
    if(!node->valid() || !current){
        return;
    }

    bool force = m_forceMemoryRead.contains(nodeId);
    m_forceMemoryRead.remove(nodeId);

    // Only the spaces that have event IDs in them
    QSet<int> spaces;
    for(const EventPin& pin : current->pins){
        spaces.insert(pin.space);
    }

    m_spacesToRead[nodeId].clear();
    m_status[nodeId].error.clear();
    setStatus(nodeId, NodeLoadState::ReadingMemory);

    for(int space : spaces){
        SegmentMemory* mem = node->segmentMemory(space);
        if(!mem){
            continue;
        }

        if(!m_connectedMemories.contains(mem)){
            m_connectedMemories.insert(mem);
            connect(mem, &SegmentMemory::dataChanged, this, [this, nodeId, space](){
                updatePinsFromMemory(nodeId, space);
            });
            connect(mem, &SegmentMemory::downloadProgress, this, [this, nodeId](uint64_t rx, uint64_t total){
                if(total > 0 && m_status.contains(nodeId)){
                    m_status[nodeId].progress = static_cast<int>(rx * 100 / total);
                }
            });
            connect(mem, &SegmentMemory::memoryReady, this, [this, nodeId, space](){
                spaceRead(nodeId, space, QString());
            });
            connect(mem, &SegmentMemory::memoryReadFailed, this, [this, nodeId, space](uint16_t, QString message){
                spaceRead(nodeId, space, message);
            });
            connect(mem, &SegmentMemory::rangeWritten, this, [this, nodeId, space](uint32_t address, int){
                writeFinished(nodeId, space, address, QString());
            });
            connect(mem, &SegmentMemory::rangeWriteFailed, this, [this, nodeId, space](uint32_t address, int, uint16_t, QString message){
                writeFinished(nodeId, space, address, message);
            });
        }

        if(mem->hasMemory() && !force){
            updatePinsFromMemory(nodeId, space);
            continue;
        }

        m_spacesToRead[nodeId].insert(space);
        mem->readAllMemory();
    }

    if(m_spacesToRead[nodeId].isEmpty()){
        setStatus(nodeId, NodeLoadState::Ready);
    }
}

void EventGraphController::spaceRead(uint64_t nodeId, int space, const QString& error){
    if(!m_spacesToRead[nodeId].remove(space)){
        // Read by somebody else - the pins are updated from dataChanged
        return;
    }

    if(!error.isEmpty()){
        LOG4CXX_ERROR_FMT(logger, "Unable to read space {:X} of node {:X}: {}", space, nodeId, error.toStdString());
        NodeLoadStatus& status = m_status[nodeId];
        if(!status.error.isEmpty()){
            status.error += '\n';
        }
        status.error += tr("Unable to read memory space 0x%1: %2")
                .arg(space, 2, 16, QLatin1Char('0'))
                .arg(error);
    }

    if(m_spacesToRead[nodeId].isEmpty()){
        if(m_status[nodeId].error.isEmpty()){
            setStatus(nodeId, NodeLoadState::Ready);
        }else{
            setStatus(nodeId, NodeLoadState::Failed, m_status[nodeId].error);
        }
    }
}

void EventGraphController::updatePinsFromMemory(uint64_t nodeId, int space){
    std::shared_ptr<LCCNode> node = m_conn->lccNodeForID(nodeId);
    SegmentMemory* mem = node->valid() ? node->segmentMemory(space) : nullptr;
    if(!mem || !mem->hasMemory()){
        return;
    }

    const QVector<uint8_t> data = mem->data();
    const uint32_t start = mem->startingAddress();
    for(const PinRef& ref : m_model.pinRefs(nodeId)){
        if(ref.space != space || ref.address < start){
            continue;
        }

        qsizetype index = ref.address - start;
        std::optional<uint64_t> eventId = eventIdFromBytes(data.mid(index, 8));
        if(eventId.has_value()){
            m_model.setPinEventId(ref, eventId.value());
        }
    }

    Q_EMIT modelChanged();
}

void EventGraphController::stageWrite(const EventWrite& write){
    m_model.stageEventId(write.pin, write.eventId);
    Q_EMIT modelChanged();
}

void EventGraphController::unstage(const PinRef& pin){
    m_model.unstage(pin);
    Q_EMIT modelChanged();
}

void EventGraphController::discardChanges(){
    m_model.clearStaged();
    Q_EMIT modelChanged();
}

int EventGraphController::applyChanges(QStringList* errors){
    QSet<uint64_t> nodes;
    int count = 0;

    const QMap<PinRef, uint64_t> staged = m_model.stagedEventIds();
    for(auto it = staged.constBegin(); it != staged.constEnd(); ++it){
        if(m_pendingWrites.contains(it.key())){
            // Already on its way
            continue;
        }

        QString error;
        if(writeEventId(EventWrite{it.key(), it.value()}, &error, false)){
            nodes.insert(it.key().nodeId);
            count++;
        }else if(errors){
            errors->push_back(tr("Unable to change %1: %2").arg(pinDescription(it.key()), error));
        }
    }

    // Once per node, after all of its writes
    for(uint64_t nodeId : nodes){
        sendUpdateComplete(nodeId);
    }

    return count;
}

void EventGraphController::sendUpdateComplete(uint64_t nodeId){
    std::shared_ptr<LCCNode> node = m_conn->lccNodeForID(nodeId);
    UpdateCompleteReply* reply = node->valid() ? node->sendUpdateComplete() : nullptr;
    if(reply){
        connect(reply, &UpdateCompleteReply::finished, this, [reply](){
            // Not every node implements this(including nodes with older versions of liblcc),
            // and the event ID has been written anyway, so just log it
            if(reply->isError()){
                LOG4CXX_WARN_FMT(logger, "Node did not accept 'update complete': {}", reply->errorMessage().toStdString());
            }
        });
    }
}

bool EventGraphController::writeEventId(const EventWrite& write, QString* error, bool updateComplete){
    auto fail = [error](const QString& message){
        if(error){
            *error = message;
        }
        return false;
    };

    if(m_pendingWrites.contains(write.pin)){
        return fail(tr("This event is already being changed"));
    }

    std::shared_ptr<LCCNode> node = m_conn->lccNodeForID(write.pin.nodeId);
    SegmentMemory* mem = node->valid() ? node->segmentMemory(write.pin.space) : nullptr;
    if(!mem || !mem->hasMemory()){
        return fail(tr("The node's memory has not been read"));
    }

    // Event IDs are big-endian
    QVector<uint8_t> bytes;
    for(int x = 7; x >= 0; x--){
        bytes.push_back((write.eventId >> (x * 8)) & 0xFF);
    }

    if(!mem->writeRange(write.pin.address, bytes)){
        return fail(tr("Unable to write to the node"));
    }
    m_pendingWrites.insert(write.pin, write.eventId);

    if(updateComplete){
        // Sent once the write is done, so that the node starts using the new event ID
        sendUpdateComplete(write.pin.nodeId);
    }

    Q_EMIT modelChanged();
    return true;
}

bool EventGraphController::isWritePending(const PinRef& pin) const{
    return m_pendingWrites.contains(pin);
}

QString EventGraphController::pinDescription(const PinRef& ref) const{
    const EventGraphNode* node = m_model.node(ref.nodeId);
    const EventPin* pin = m_model.pin(ref);
    if(!node || !pin){
        return QString();
    }

    QString nodeName = node->name.isEmpty() ? node->model : node->name;
    if(nodeName.isEmpty()){
        nodeName = QString::number(ref.nodeId, 16);
    }
    return QString("%1: %2").arg(nodeName, pin->path());
}

void EventGraphController::writeFinished(uint64_t nodeId, int space, uint32_t address, const QString& error){
    const PinRef ref{nodeId, space, address};
    if(!m_pendingWrites.remove(ref)){
        // Written by something else, e.g. a CDI editor.  The pins are updated from dataChanged.
        return;
    }

    if(!error.isEmpty()){
        Q_EMIT writeFailed(tr("Unable to change %1: %2").arg(pinDescription(ref), error));
    }else{
        // What the node produces/consumes has changed
        struct lcc_node_info* info = m_conn->lccNodeInfoForID(nodeId);
        if(info){
            lcc_node_refresh_events_produced(info);
            lcc_node_refresh_events_consumed(info);
        }
    }

    Q_EMIT modelChanged();
}

void EventGraphController::incomingEvent(uint16_t sourceAlias, uint64_t eventId){
    uint64_t nodeId = m_conn->nodeIdForAlias(sourceAlias);
    QVector<PinRef> sendingPins;
    if(nodeId != 0){
        sendingPins = m_model.eventObserved(nodeId, eventId);
    }

    if(sendingPins.isEmpty() && m_model.pinsWithEvent(eventId).isEmpty()){
        // Not in anybody's CDI(e.g. a well-known event), so it's not in the graph
        return;
    }

    Q_EMIT eventSeen(nodeId, eventId, sendingPins);
    Q_EMIT modelChanged();
}

void EventGraphController::setStatus(uint64_t nodeId, NodeLoadState state, const QString& error){
    NodeLoadStatus& status = m_status[nodeId];
    if(status.state != state){
        status.progress = 0;
    }
    status.state = state;
    if(!error.isEmpty()){
        status.error = error;
    }
    Q_EMIT modelChanged();
}
