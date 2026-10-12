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

    // Changes to pins that the node no longer has can't be applied
    for(auto it = m_staged.begin(); it != m_staged.end(); ){
        if(it.key().nodeId == node.nodeId && !pin(it.key())){
            it = m_staged.erase(it);
        }else{
            ++it;
        }
    }

    applyLearned(stored);
    applyIdentified(stored);
}

void EventGraphModel::removeNode(uint64_t nodeId){
    m_nodes.remove(nodeId);
    m_identified.remove(nodeId);
    for(auto it = m_staged.begin(); it != m_staged.end(); ){
        if(it.key().nodeId == nodeId){
            it = m_staged.erase(it);
        }else{
            ++it;
        }
    }
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
    if(m_staged.contains(ref) && m_staged.value(ref) == eventId){
        // The change has been written
        m_staged.remove(ref);
    }
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
        bool staged;

        bool operator<(const GroupPin& other) const{ return ref < other.ref; }
    };

    // Group the pins by event ID
    QMap<uint64_t, QVector<GroupPin>> groups;
    for(const EventGraphNode& n : m_nodes){
        for(const EventPin& p : n.pins){
            const PinRef ref{n.nodeId, p.space, p.address};
            auto staged = m_staged.constFind(ref);
            const bool isStaged = staged != m_staged.constEnd();
            std::optional<uint64_t> id = isStaged ? std::optional<uint64_t>(staged.value()) : p.eventId;
            if(id.has_value() && id.value() != NULL_EVENT_ID){
                groups[id.value()].push_back(
                            GroupPin{ref, p.direction == PinDirection::Producer, isStaged});
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

        QVector<GroupPin> producers;
        QVector<GroupPin> others;
        for(const GroupPin& p : pins){
            (p.producer ? producers : others).push_back(p);
        }

        if(!producers.isEmpty() && !others.isEmpty()){
            // Every producer sends to everything else
            for(const GroupPin& producer : producers){
                for(const GroupPin& other : others){
                    result.push_back(EventLink{it.key(), producer.ref, other.ref, true,
                                               producer.staged || other.staged});
                }
            }
        }else{
            // We don't know who sends the event(or everybody does), so connect
            // everything to one pin so that the group is still shown as connected
            const GroupPin& hub = pins.first();
            for(int x = 1; x < pins.size(); x++){
                result.push_back(EventLink{it.key(), hub.ref, pins[x].ref, false,
                                           hub.staged || pins[x].staged});
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
    // Use what is shown in the graph, so changes can be made on top of other changes
    const std::optional<uint64_t> fromEventId = eventId(from);
    const std::optional<uint64_t> toEventId = eventId(to);
    if(!fromEventId.has_value() || !toEventId.has_value()){
        return ConnectResult::ValueUnknown;
    }

    uint64_t fromId = fromEventId.value();
    uint64_t toId = toEventId.value();
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

void EventGraphModel::stageEventId(const PinRef& ref, uint64_t eventId){
    const EventPin* p = pin(ref);
    if(!p){
        return;
    }

    if(p->eventId == eventId){
        // That's what the node has already
        m_staged.remove(ref);
    }else{
        m_staged.insert(ref, eventId);
    }
}

void EventGraphModel::unstage(const PinRef& ref){
    m_staged.remove(ref);
}

void EventGraphModel::clearStaged(){
    m_staged.clear();
}

QMap<PinRef, uint64_t> EventGraphModel::stagedEventIds() const{
    return m_staged;
}

bool EventGraphModel::isStaged(const PinRef& ref) const{
    return m_staged.contains(ref);
}

std::optional<uint64_t> EventGraphModel::eventId(const PinRef& ref) const{
    auto staged = m_staged.constFind(ref);
    if(staged != m_staged.constEnd()){
        return staged.value();
    }

    const EventPin* p = pin(ref);
    return p ? p->eventId : std::nullopt;
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

QVector<PinTreeGroup> buildPinTree(const QVector<EventPin>& pins){
    PinTreeGroup root;

    for(int pinIndex = 0; pinIndex < pins.size(); pinIndex++){
        const EventPin& pin = pins[pinIndex];
        PinTreeGroup* group = &root;
        group->totalPins++;

        for(int depth = 0; depth < pin.groupPath.size(); depth++){
            const QString& label = pin.groupPath[depth];

            // Pins are normally in the same group as the previous pin, so check the last one first
            PinTreeGroup* child = nullptr;
            for(int x = group->children.size() - 1; x >= 0; x--){
                if(group->children[x].label == label){
                    child = &group->children[x];
                    break;
                }
            }
            if(!child){
                PinTreeGroup newGroup;
                newGroup.label = label;
                newGroup.path = pin.groupPath.mid(0, depth + 1);
                group->children.push_back(newGroup);
                child = &group->children.last();
            }

            group = child;
            group->totalPins++;
        }

        group->pins.push_back(pinIndex);
    }

    return root.children;
}

EventSearch::EventSearch(const QString& text) :
    m_text(text.trimmed())
{
    m_lowerText = m_text.toLower();

    QString hex = m_text;
    hex.remove('.').remove(' ').remove(':');
    bool allHex = !hex.isEmpty();
    for(QChar c : hex){
        if(!c.isDigit() && !(c.toLower() >= 'a' && c.toLower() <= 'f')){
            allHex = false;
            break;
        }
    }
    if(allHex){
        m_hex = hex.toUpper();
    }
}

bool EventSearch::isEmpty() const{
    return m_text.isEmpty();
}

QString EventSearch::text() const{
    return m_text;
}

bool EventSearch::matches(const EventPin& pin) const{
    if(isEmpty()){
        return false;
    }

    if(!m_hex.isEmpty() && pin.eventId.has_value()){
        QString eventHex = QString("%1").arg(pin.eventId.value(), 16, 16, QLatin1Char('0')).toUpper();
        if(eventHex.contains(m_hex)){
            return true;
        }
    }

    return pin.path().toLower().contains(m_lowerText);
}
