/* SPDX-License-Identifier: GPL-2.0 */
#include "eventgraphmodel.h"
#include "lcc/cdi/eventidtype.h"

#include <algorithm>
#include <tuple>

bool PinRef::operator==(const PinRef& other) const{
    return nodeId == other.nodeId && space == other.space && address == other.address;
}

bool PinRef::operator!=(const PinRef& other) const{
    return !(*this == other);
}

bool PinRef::operator<(const PinRef& other) const{
    return std::tie(nodeId, space, address) < std::tie(other.nodeId, other.space, other.address);
}

QString EventPin::path() const{
    QStringList parts = groupPath;
    parts.push_back(label);
    return parts.join('/');
}

QString EventGraphNode::modelKey() const{
    return manufacturer + '\n' + model;
}

void EventGraphModel::setNode(const EventGraphNode& node){
    EventGraphNode& stored = m_nodes[node.nodeId];
    stored = node;
    applyLearned(stored);
    applyIdentified(stored);
}

void EventGraphModel::removeNode(uint64_t nodeId){
    m_nodes.remove(nodeId);
    m_identified.remove(nodeId);
}

QVector<uint64_t> EventGraphModel::nodeIds() const{
    return m_nodes.keys().toVector();
}

const EventGraphNode* EventGraphModel::node(uint64_t nodeId) const{
    auto it = m_nodes.constFind(nodeId);
    return it == m_nodes.constEnd() ? nullptr : &it.value();
}

const EventPin* EventGraphModel::pin(const PinRef& ref) const{
    const EventGraphNode* n = node(ref.nodeId);
    if(!n){
        return nullptr;
    }

    for(const EventPin& p : n->pins){
        if(p.space == ref.space && p.address == ref.address){
            return &p;
        }
    }
    return nullptr;
}

EventPin* EventGraphModel::findPin(const PinRef& ref){
    auto it = m_nodes.find(ref.nodeId);
    if(it == m_nodes.end()){
        return nullptr;
    }

    for(EventPin& p : it->pins){
        if(p.space == ref.space && p.address == ref.address){
            return &p;
        }
    }
    return nullptr;
}

QVector<PinRef> EventGraphModel::pinRefs(uint64_t nodeId) const{
    QVector<PinRef> refs;
    const EventGraphNode* n = node(nodeId);
    if(n){
        for(const EventPin& p : n->pins){
            refs.push_back(PinRef{nodeId, p.space, p.address});
        }
    }
    return refs;
}

bool EventGraphModel::setPinEventId(const PinRef& ref, uint64_t eventId){
    EventPin* p = findPin(ref);
    if(!p){
        return false;
    }

    p->eventId = eventId;
    applyIdentified(m_nodes[ref.nodeId]);
    return true;
}

void EventGraphModel::setIdentifiedEvents(uint64_t nodeId, const QVector<uint64_t>& produced, const QVector<uint64_t>& consumed){
    m_identified[nodeId] = IdentifiedEvents{produced, consumed};

    auto it = m_nodes.find(nodeId);
    if(it != m_nodes.end()){
        applyIdentified(it.value());
    }
}

QVector<PinRef> EventGraphModel::eventObserved(uint64_t nodeId, uint64_t eventId){
    QVector<PinRef> refs;
    auto it = m_nodes.find(nodeId);
    if(it == m_nodes.end() || eventId == NULL_EVENT_ID){
        return refs;
    }

    for(EventPin& p : it->pins){
        if(p.eventId == eventId){
            setDirection(it.value(), p, PinDirection::Producer, DirectionSource::Observed);
            refs.push_back(PinRef{nodeId, p.space, p.address});
        }
    }
    return refs;
}

QVector<PinRef> EventGraphModel::pinsWithEvent(uint64_t eventId) const{
    QVector<PinRef> refs;
    if(eventId == NULL_EVENT_ID){
        return refs;
    }

    for(const EventGraphNode& n : m_nodes){
        for(const EventPin& p : n.pins){
            if(p.eventId == eventId){
                refs.push_back(PinRef{n.nodeId, p.space, p.address});
            }
        }
    }
    std::sort(refs.begin(), refs.end());
    return refs;
}

QVector<EventLink> EventGraphModel::links() const{
    struct GroupPin {
        PinRef ref;
        bool producer;

        bool operator<(const GroupPin& other) const{ return ref < other.ref; }
    };

    // Group the pins by event ID
    QMap<uint64_t, QVector<GroupPin>> groups;
    for(const EventGraphNode& n : m_nodes){
        for(const EventPin& p : n.pins){
            if(p.eventId.has_value() && p.eventId.value() != NULL_EVENT_ID){
                groups[p.eventId.value()].push_back(
                            GroupPin{PinRef{n.nodeId, p.space, p.address},
                                     p.direction == PinDirection::Producer});
            }
        }
    }

    QVector<EventLink> result;
    for(auto it = groups.begin(); it != groups.end(); ++it){
        QVector<GroupPin>& pins = it.value();
        if(pins.size() < 2){
            continue;
        }
        std::sort(pins.begin(), pins.end());

        QVector<PinRef> producers;
        QVector<PinRef> others;
        for(const GroupPin& p : pins){
            (p.producer ? producers : others).push_back(p.ref);
        }

        if(!producers.isEmpty() && !others.isEmpty()){
            // Every producer sends to everything else
            for(const PinRef& producer : producers){
                for(const PinRef& other : others){
                    result.push_back(EventLink{it.key(), producer, other, true});
                }
            }
        }else{
            // We don't know who sends the event(or everybody does), so connect
            // everything to one pin so that the group is still shown as connected
            const PinRef& hub = pins.first().ref;
            for(int x = 1; x < pins.size(); x++){
                result.push_back(EventLink{it.key(), hub, pins[x].ref, false});
            }
        }
    }

    return result;
}

ConnectResult EventGraphModel::connect(const PinRef& from, const PinRef& to, EventWrite* write) const{
    const EventPin* fromPin = pin(from);
    const EventPin* toPin = pin(to);

    if(!fromPin || !toPin){
        return ConnectResult::InvalidPin;
    }
    if(from == to){
        return ConnectResult::SamePin;
    }
    if(!fromPin->eventId.has_value() || !toPin->eventId.has_value()){
        return ConnectResult::ValueUnknown;
    }

    uint64_t fromId = fromPin->eventId.value();
    uint64_t toId = toPin->eventId.value();
    if(fromId == NULL_EVENT_ID && toId == NULL_EVENT_ID){
        return ConnectResult::BothEmpty;
    }
    if(fromId == toId){
        return ConnectResult::AlreadyConnected;
    }

    // Copy from 'from' into 'to', unless 'to' is the producer, or 'from' has nothing to copy
    bool copyFromTo = toPin->direction == PinDirection::Producer &&
            fromPin->direction != PinDirection::Producer &&
            toId != NULL_EVENT_ID;
    if(fromId == NULL_EVENT_ID){
        copyFromTo = true;
    }

    if(write){
        if(copyFromTo){
            *write = EventWrite{from, toId};
        }else{
            *write = EventWrite{to, fromId};
        }
    }
    return ConnectResult::Ok;
}

EventWrite EventGraphModel::disconnect(const PinRef& pin) const{
    return EventWrite{pin, NULL_EVENT_ID};
}

int EventGraphModel::linkCount(const PinRef& ref) const{
    int count = 0;
    for(const EventLink& link : links()){
        if(link.from == ref || link.to == ref){
            count++;
        }
    }
    return count;
}

QMap<QString, QMap<QString, PinDirection>> EventGraphModel::learnedDirections() const{
    return m_learned;
}

void EventGraphModel::setLearnedDirections(const QMap<QString, QMap<QString, PinDirection>>& learned){
    m_learned = learned;
    for(EventGraphNode& n : m_nodes){
        applyLearned(n);
    }
}

void EventGraphModel::setDirection(EventGraphNode& node, EventPin& pin, PinDirection direction, DirectionSource source){
    if(source < pin.directionSource){
        // We already know better
        return;
    }

    pin.direction = direction;
    pin.directionSource = source;

    if(source > DirectionSource::Learned && direction != PinDirection::Unknown){
        // Remember this for other nodes of the same model
        m_learned[node.modelKey()][pin.path()] = direction;
    }
}

void EventGraphModel::applyIdentified(EventGraphNode& node){
    auto it = m_identified.constFind(node.nodeId);
    if(it == m_identified.constEnd()){
        return;
    }

    for(EventPin& p : node.pins){
        if(!p.eventId.has_value() || p.eventId.value() == NULL_EVENT_ID){
            continue;
        }

        bool produced = it->produced.contains(p.eventId.value());
        bool consumed = it->consumed.contains(p.eventId.value());
        if(produced == consumed){
            // Either not listed, or listed as both, in which case we can't tell
            // which field it is for
            continue;
        }

        setDirection(node, p, produced ? PinDirection::Producer : PinDirection::Consumer,
                     DirectionSource::Identified);
    }
}

void EventGraphModel::applyLearned(EventGraphNode& node){
    auto modelIt = m_learned.constFind(node.modelKey());
    if(modelIt == m_learned.constEnd()){
        return;
    }

    for(EventPin& p : node.pins){
        auto pinIt = modelIt->constFind(p.path());
        if(pinIt != modelIt->constEnd()){
            setDirection(node, p, pinIt.value(), DirectionSource::Learned);
        }
    }
}

QVector<EventPin> eventPinsFromLayout(const QVector<CdiLayoutGroup>& layout){
    QVector<EventPin> pins;

    cdiForEachField(layout, [&pins](const CdiField& field, const QStringList& groupPath){
        auto eventType = std::get_if<std::shared_ptr<EventIDType>>(&field.variant);
        if(!eventType){
            return;
        }

        EventPin p;
        p.space = field.space;
        p.address = field.address;
        p.label = (*eventType)->name();
        p.groupPath = groupPath;
        pins.push_back(p);
    });

    return pins;
}

std::optional<uint64_t> eventIdFromBytes(const QVector<uint8_t>& bytes){
    if(bytes.size() != 8){
        return std::nullopt;
    }

    uint64_t value = 0;
    for(uint8_t b : bytes){
        value = (value << 8) | b;
    }
    return value;
}
