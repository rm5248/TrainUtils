/* SPDX-License-Identifier: GPL-2.0 */
#ifndef EVENTGRAPHMODEL_H
#define EVENTGRAPHMODEL_H

#include <QMap>
#include <QString>
#include <QStringList>
#include <QVector>
#include <cstdint>
#include <optional>

#include "lcc/cdi/cdilayout.h"

/**
 * Whether a node sends(produces) or listens for(consumes) the event in a pin.
 * The CDI does not say, so this is figured out from what we can see on the network.
 */
enum class PinDirection {
    Unknown,
    Producer,
    Consumer,
};

/**
 * Where a pin's direction came from.  A direction from a more reliable source
 * replaces one from a less reliable source, never the other way around.
 */
enum class DirectionSource {
    None,
    /** A node of the same model had this direction for the same field */
    Learned,
    /** The node said that it produces/consumes the event(Producer/Consumer Identified) */
    Identified,
    /** The node was seen sending the event */
    Observed,
};

/**
 * Identifies a pin: a node, and where in that node's memory the event ID is.
 */
struct PinRef {
    uint64_t nodeId = 0;
    int space = 0;
    uint32_t address = 0;

    bool operator==(const PinRef& other) const;
    bool operator!=(const PinRef& other) const;
    bool operator<(const PinRef& other) const;
};

/**
 * An event ID field in a node's configuration.
 */
struct EventPin {
    int space = 0;
    uint32_t address = 0;
    /** The name of the field in the CDI */
    QString label;
    /** The segment and groups that the field is in, e.g. {"Port I/O", "Line 3"} */
    QStringList groupPath;
    /** The event ID in the node's memory.  Not set until the memory has been read. */
    std::optional<uint64_t> eventId;
    PinDirection direction = PinDirection::Unknown;
    DirectionSource directionSource = DirectionSource::None;

    /** Identifies this field within a CDI, e.g. "Port I/O/Line 3/Event when on" */
    QString path() const;
};

/**
 * A node on the network, and its event ID fields.
 */
struct EventGraphNode {
    uint64_t nodeId = 0;
    QString manufacturer;
    QString model;
    /** The user-assigned name of the node, if there is one */
    QString name;
    QVector<EventPin> pins;

    /** Nodes with the same model key have the same CDI layout */
    QString modelKey() const;
};

/**
 * Two pins that have the same event ID.
 */
struct EventLink {
    uint64_t eventId = 0;
    PinRef from;
    PinRef to;
    /** true if 'from' is known to be the producer of the event */
    bool directed = false;
};

/**
 * A value to write to a node in order to connect or disconnect pins.
 */
struct EventWrite {
    PinRef pin;
    uint64_t eventId = 0;
};

enum class ConnectResult {
    /** The write has been filled in */
    Ok,
    InvalidPin,
    SamePin,
    /** The event ID of one of the pins has not been read yet */
    ValueUnknown,
    AlreadyConnected,
    /** Both pins have the null event ID, so there is nothing to connect them with */
    BothEmpty,
};

/**
 * The nodes on the network and how their events are connected.  Links are not
 * stored: two pins are linked if they have the same(non-null) event ID, so the
 * links always match what is actually in the nodes.
 *
 * This class does not do any I/O: it says what needs to be written, and is told
 * what has been read.
 */
class EventGraphModel
{
public:
    static constexpr uint64_t NULL_EVENT_ID = 0;

    /**
     * Add a node, or replace the node with the same ID.  Pins that don't have a
     * direction get one learned from other nodes of the same model, if known.
     */
    void setNode(const EventGraphNode& node);
    void removeNode(uint64_t nodeId);
    QVector<uint64_t> nodeIds() const;
    const EventGraphNode* node(uint64_t nodeId) const;
    const EventPin* pin(const PinRef& ref) const;
    QVector<PinRef> pinRefs(uint64_t nodeId) const;

    /**
     * Set the event ID of a pin, after it was read from or written to the node.
     *
     * @return false if there is no such pin
     */
    bool setPinEventId(const PinRef& ref, uint64_t eventId);

    /**
     * The events that the node says that it produces and consumes.  These are
     * remembered, so pins whose event ID is set later still get the direction.
     * Not all nodes provide this.
     */
    void setIdentifiedEvents(uint64_t nodeId, const QVector<uint64_t>& produced, const QVector<uint64_t>& consumed);

    /**
     * A node sent an event.  Its pins with that event ID are producers.
     *
     * @return The pins of the node that have the event ID(empty if the event is
     * not in the node's CDI, e.g. a well-known event)
     */
    QVector<PinRef> eventObserved(uint64_t nodeId, uint64_t eventId);

    /** All of the pins with the given(non-null) event ID */
    QVector<PinRef> pinsWithEvent(uint64_t eventId) const;

    QVector<EventLink> links() const;

    /**
     * Figure out what to write to connect two pins: the event ID of one pin is
     * copied into the other.  The event ID of 'from' is used, unless it is the null
     * event ID, or 'to' is a producer and 'from' is not.
     */
    ConnectResult connect(const PinRef& from, const PinRef& to, EventWrite* write) const;

    /**
     * What to write to disconnect a pin: the null event ID.  Note that this
     * disconnects the pin from every pin with the same event ID.
     */
    EventWrite disconnect(const PinRef& pin) const;

    /** How many links a pin is part of(and would be removed by disconnecting it) */
    int linkCount(const PinRef& pin) const;

    /**
     * Directions that have been learned, by model key and then field path.
     * Used to save and restore what has been learned.
     */
    QMap<QString, QMap<QString, PinDirection>> learnedDirections() const;
    void setLearnedDirections(const QMap<QString, QMap<QString, PinDirection>>& learned);

private:
    struct IdentifiedEvents {
        QVector<uint64_t> produced;
        QVector<uint64_t> consumed;
    };

    EventPin* findPin(const PinRef& ref);
    void setDirection(EventGraphNode& node, EventPin& pin, PinDirection direction, DirectionSource source);
    void applyIdentified(EventGraphNode& node);
    void applyLearned(EventGraphNode& node);

    QMap<uint64_t, EventGraphNode> m_nodes;
    QMap<uint64_t, IdentifiedEvents> m_identified;
    QMap<QString, QMap<QString, PinDirection>> m_learned;
};

/**
 * Create a pin for every event ID field in a CDI.  The event IDs are not set.
 */
QVector<EventPin> eventPinsFromLayout(const QVector<CdiLayoutGroup>& layout);

/**
 * Convert 8 bytes of memory(big-endian) to an event ID
 */
std::optional<uint64_t> eventIdFromBytes(const QVector<uint8_t>& bytes);

#endif // EVENTGRAPHMODEL_H
