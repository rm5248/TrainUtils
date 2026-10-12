/* SPDX-License-Identifier: GPL-2.0 */
#ifndef EVENTGRAPHCONTROLLER_H
#define EVENTGRAPHCONTROLLER_H

#include <QObject>
#include <QMap>
#include <QSet>
#include <QString>
#include <memory>

#include "eventgraphmodel.h"

class LCCConnection;
class LCCNode;
class SegmentMemory;

/**
 * How far along we are in figuring out the event IDs of a node.
 */
enum class NodeLoadState {
    /** Waiting for the node to tell us what it is(simple node information) */
    WaitingForInfo,
    ReadingCdi,
    /** Reading the event IDs from the node's configuration memory */
    ReadingMemory,
    Ready,
    /** The node does not have a CDI, so we don't know its event IDs */
    NoCdi,
    Failed,
};

struct NodeLoadStatus {
    NodeLoadState state = NodeLoadState::WaitingForInfo;
    /** 0-100, when reading */
    int progress = 0;
    /** Details if the state is Failed */
    QString error;
};

/**
 * Fills in an EventGraphModel from the nodes on an LCC network: finds the nodes,
 * reads their CDIs and the event IDs from their memory, and keeps the model up to
 * date as things change.
 */
class EventGraphController : public QObject
{
    Q_OBJECT
public:
    explicit EventGraphController(std::shared_ptr<LCCConnection> conn, QObject* parent = nullptr);

    std::shared_ptr<LCCConnection> connection() const;
    EventGraphModel& model();
    NodeLoadStatus loadStatus(uint64_t nodeId) const;

    /**
     * Find all of the nodes on the network and read their event IDs.
     */
    void start();

    /**
     * Read the event IDs of the node again.
     */
    void reloadNode(uint64_t nodeId);

    /**
     * Change an event ID in the graph(to connect or disconnect pins), without
     * writing it to the node.  See applyChanges().
     */
    void stageWrite(const EventWrite& write);
    void unstage(const PinRef& pin);
    void discardChanges();

    /**
     * Write all of the staged changes to the nodes.  A change is removed from the
     * staged changes once the node has it, so failed changes are left to try again.
     *
     * @param errors Why changes could not be written
     * @return How many changes are being written
     */
    int applyChanges(QStringList* errors = nullptr);

    /**
     * Write an event ID to a node.  The model is updated once the node has it.
     *
     * @param error Why the write could not be started
     * @param sendUpdateComplete Tell the node that we are done after writing
     * @return false if the write could not be started
     */
    bool writeEventId(const EventWrite& write, QString* error = nullptr, bool sendUpdateComplete = true);

    /** An event ID is being written to the pin */
    bool isWritePending(const PinRef& pin) const;

    /** A description of a pin for the user, e.g. "Signal Board: Port I/O/Line 3/Event when on" */
    QString pinDescription(const PinRef& pin) const;

Q_SIGNALS:
    /** Something in the model changed */
    void modelChanged();

    /**
     * A node sent an event that is in the graph.
     *
     * @param nodeId The node that sent it(0 if not known)
     * @param eventId
     * @param sendingPins The pins of the sending node that have the event ID
     */
    void eventSeen(uint64_t nodeId, uint64_t eventId, QVector<PinRef> sendingPins);

    /** Writing to a node failed */
    void writeFailed(QString message);

private Q_SLOTS:
    void nodeDiscovered(uint64_t nodeId);
    void nodeInformationUpdated(uint64_t nodeId);
    void incomingEvent(uint16_t sourceAlias, uint64_t eventId);

private:
    void updateNodeInformation(uint64_t nodeId);
    /**
     * @param force Read the CDI from the node, even if we already have it
     */
    void readCdi(uint64_t nodeId, bool force = false);
    void cdiAvailable(uint64_t nodeId);
    void readEventIds(uint64_t nodeId);
    void spaceRead(uint64_t nodeId, int space, const QString& error);
    void updatePinsFromMemory(uint64_t nodeId, int space);
    void writeFinished(uint64_t nodeId, int space, uint32_t address, const QString& error);
    void sendUpdateComplete(uint64_t nodeId);
    void setStatus(uint64_t nodeId, NodeLoadState state, const QString& error = QString());
    QString cdiCacheKey(uint64_t nodeId);

    std::shared_ptr<LCCConnection> m_conn;
    EventGraphModel m_model;
    QMap<uint64_t, NodeLoadStatus> m_status;
    /** Nodes whose LCCNode/SegmentMemory signals we are already connected to */
    QSet<uint64_t> m_connectedNodes;
    QSet<SegmentMemory*> m_connectedMemories;
    /** Nodes whose memory should be read again, even if we have it */
    QSet<uint64_t> m_forceMemoryRead;
    /** Spaces that we still need to read, per node */
    QMap<uint64_t, QSet<int>> m_spacesToRead;
    /** Event IDs being written to pins */
    QMap<PinRef, uint64_t> m_pendingWrites;
};

#endif // EVENTGRAPHCONTROLLER_H
