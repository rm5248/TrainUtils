/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Tests for the CDI layout and the event graph model.
 */
#include <cstdio>
#include <cstring>

#include <QXmlStreamReader>

#include "lcc/cdi/cdi.h"
#include "lcc/cdi/cdilayout.h"
#include "lcc/cdi/cdivariant.h"
#include "lcc/eventgraph/eventgraphmodel.h"
#include "lcc/eventgraph/eventgraphstorage.h"

#define CHECK(cond) do{ \
    if(!(cond)){ \
        printf("%s:%d: check failed: %s\n", __FILE__, __LINE__, #cond); \
        return 1; \
    } \
}while(0)

/*
 * Addresses(space 253, origin 16):
 *   Version           16-17
 *   Line 1           18: Event when on 20, Event when off 28, Mode 36
 *   Line 2           37: Event when on 39, Event when off 47, Mode 55
 *   (padding group)   56-59
 *   Name              60-67
 *   Single           68: Only 68
 */
static const char* test_cdi =
        "<?xml version='1.0'?>"
        "<cdi>"
        "<identification><manufacturer>TrainUtils</manufacturer><model>Test</model></identification>"
        "<segment space='253' origin='16'>"
        "<name>Port I/O</name>"
        "<int size='2'><name>Version</name></int>"
        "<group replication='2'>"
        "<name>Lines</name>"
        "<repname>Line</repname>"
        "<eventid offset='2'><name>Event when on</name></eventid>"
        "<eventid><name>Event when off</name></eventid>"
        "<int><name>Mode</name></int>"
        "</group>"
        "<group offset='4'/>"
        "<string size='8'><name>Name</name></string>"
        "<group><name>Single</name><eventid><name>Only</name></eventid></group>"
        "</segment>"
        "<segment space='251'>"
        "<name>Node ID</name>"
        "<string size='63'><name>User name</name></string>"
        "</segment>"
        "</cdi>";

static const uint64_t NODE_A = 0x050101012200;
static const uint64_t NODE_B = 0x050101012201;
static const uint64_t NODE_C = 0x050101012202;
static const uint64_t EVENT_1 = 0x0501010122000001ull;
static const uint64_t EVENT_2 = 0x0501010122000002ull;

static CDI parseTestCdi(){
    QXmlStreamReader reader{QByteArray(test_cdi)};
    return CDI::createFromXML(&reader);
}

/*
 * A node with 3 pins at addresses 0, 8 and 16 of space 253
 */
static EventGraphNode makeNode(uint64_t nodeId, const QString& model = "Model"){
    EventGraphNode node;
    node.nodeId = nodeId;
    node.manufacturer = "TrainUtils";
    node.model = model;
    for(int x = 0; x < 3; x++){
        EventPin pin;
        pin.space = 253;
        pin.address = x * 8;
        pin.label = QString("Event %1").arg(x);
        pin.groupPath = QStringList{"Events"};
        node.pins.push_back(pin);
    }
    return node;
}

static PinRef ref(uint64_t nodeId, int pinIndex){
    return PinRef{nodeId, 253, static_cast<uint32_t>(pinIndex * 8)};
}

static int layout_addresses(){
    QVector<CdiLayoutGroup> layout = cdiLayout(parseTestCdi());

    CHECK(layout.size() == 2);

    const CdiLayoutGroup& io = layout[0];
    CHECK(io.label == "Port I/O");
    CHECK(io.space == 253);
    CHECK(io.address == 16);
    CHECK(io.fields.size() == 2);
    CHECK(cdiVariantName(io.fields[0].variant) == "Version");
    CHECK(io.fields[0].address == 16);
    CHECK(cdiVariantName(io.fields[1].variant) == "Name");
    CHECK(io.fields[1].address == 60);

    // The padding group is not shown
    CHECK(io.children.size() == 3);

    const CdiLayoutGroup& line1 = io.children[0];
    CHECK(line1.label == "Line 1");
    CHECK(line1.address == 18);
    CHECK(line1.fields.size() == 3);
    CHECK(line1.fields[0].address == 20);
    CHECK(line1.fields[1].address == 28);
    CHECK(line1.fields[2].address == 36);

    const CdiLayoutGroup& line2 = io.children[1];
    CHECK(line2.label == "Line 2");
    CHECK(line2.address == 37);
    CHECK(line2.fields[0].address == 39);
    CHECK(line2.fields[1].address == 47);
    CHECK(line2.fields[2].address == 55);
    CHECK(line2.fields[0].space == 253);

    const CdiLayoutGroup& single = io.children[2];
    CHECK(single.label == "Single");
    CHECK(single.fields.size() == 1);
    CHECK(single.fields[0].address == 68);

    const CdiLayoutGroup& nodeId = layout[1];
    CHECK(nodeId.label == "Node ID");
    CHECK(nodeId.space == 251);
    CHECK(nodeId.fields.size() == 1);
    CHECK(nodeId.fields[0].address == 0);

    return 0;
}

static int pins_from_layout(){
    QVector<EventPin> pins = eventPinsFromLayout(cdiLayout(parseTestCdi()));

    CHECK(pins.size() == 5);
    CHECK(pins[0].address == 20);
    CHECK(pins[0].space == 253);
    CHECK(pins[0].label == "Event when on");
    CHECK(pins[0].path() == "Port I/O/Line 1/Event when on");
    CHECK(pins[1].address == 28);
    CHECK(pins[2].address == 39);
    CHECK(pins[2].path() == "Port I/O/Line 2/Event when on");
    CHECK(pins[3].address == 47);
    CHECK(pins[4].address == 68);
    CHECK(pins[4].path() == "Port I/O/Single/Only");

    // Nothing is known about the pins yet
    for(const EventPin& pin : pins){
        CHECK(!pin.eventId.has_value());
        CHECK(pin.direction == PinDirection::Unknown);
    }

    return 0;
}

static int event_id_from_bytes(){
    CHECK(eventIdFromBytes({0x05, 0x01, 0x01, 0x01, 0x22, 0x00, 0x00, 0x01}) == EVENT_1);
    CHECK(!eventIdFromBytes({0x05, 0x01}).has_value());
    return 0;
}

/*
 * A producer is linked to each pin that has the same event ID
 */
static int links_producer(){
    EventGraphModel model;
    model.setNode(makeNode(NODE_A));
    model.setNode(makeNode(NODE_B));
    model.setNode(makeNode(NODE_C));

    model.setPinEventId(ref(NODE_A, 0), EVENT_1);
    model.setPinEventId(ref(NODE_B, 1), EVENT_1);
    model.setPinEventId(ref(NODE_C, 2), EVENT_1);
    model.eventObserved(NODE_B, EVENT_1);

    QVector<EventLink> links = model.links();
    CHECK(links.size() == 2);
    for(const EventLink& link : links){
        CHECK(link.directed);
        CHECK(link.eventId == EVENT_1);
        CHECK(link.from == ref(NODE_B, 1));
    }
    CHECK(links[0].to == ref(NODE_A, 0));
    CHECK(links[1].to == ref(NODE_C, 2));

    return 0;
}

/*
 * With no known producer, the pins are linked through the first pin
 */
static int links_unknown_direction(){
    EventGraphModel model;
    model.setNode(makeNode(NODE_A));
    model.setNode(makeNode(NODE_B));
    model.setNode(makeNode(NODE_C));

    model.setPinEventId(ref(NODE_C, 0), EVENT_1);
    model.setPinEventId(ref(NODE_B, 2), EVENT_1);
    model.setPinEventId(ref(NODE_A, 1), EVENT_1);

    QVector<EventLink> links = model.links();
    CHECK(links.size() == 2);
    for(const EventLink& link : links){
        CHECK(!link.directed);
        CHECK(link.from == ref(NODE_A, 1));
    }
    CHECK(links[0].to == ref(NODE_B, 2));
    CHECK(links[1].to == ref(NODE_C, 0));

    return 0;
}

/*
 * Null and unread event IDs are never linked, and neither is a pin on its own
 */
static int links_ignored(){
    EventGraphModel model;
    model.setNode(makeNode(NODE_A));
    model.setNode(makeNode(NODE_B));

    model.setPinEventId(ref(NODE_A, 0), EventGraphModel::NULL_EVENT_ID);
    model.setPinEventId(ref(NODE_B, 0), EventGraphModel::NULL_EVENT_ID);
    model.setPinEventId(ref(NODE_A, 1), EVENT_2);
    // NODE_B pin 1 and both pin 2s are not read yet

    CHECK(model.links().isEmpty());
    CHECK(model.pinsWithEvent(EventGraphModel::NULL_EVENT_ID).isEmpty());
    CHECK(model.pinsWithEvent(EVENT_2).size() == 1);

    return 0;
}

static int connect_copies_from_source(){
    EventGraphModel model;
    model.setNode(makeNode(NODE_A));
    model.setNode(makeNode(NODE_B));
    model.setPinEventId(ref(NODE_A, 0), EVENT_1);
    model.setPinEventId(ref(NODE_B, 0), EVENT_2);

    EventWrite write;
    CHECK(model.connect(ref(NODE_A, 0), ref(NODE_B, 0), &write) == ConnectResult::Ok);
    CHECK(write.pin == ref(NODE_B, 0));
    CHECK(write.eventId == EVENT_1);

    return 0;
}

static int connect_producer_wins(){
    EventGraphModel model;
    model.setNode(makeNode(NODE_A));
    model.setNode(makeNode(NODE_B));
    model.setPinEventId(ref(NODE_A, 0), EVENT_1);
    model.setPinEventId(ref(NODE_B, 0), EVENT_2);
    model.eventObserved(NODE_B, EVENT_2);

    // Dragging from a consumer to a producer copies the producer's event
    EventWrite write;
    CHECK(model.connect(ref(NODE_A, 0), ref(NODE_B, 0), &write) == ConnectResult::Ok);
    CHECK(write.pin == ref(NODE_A, 0));
    CHECK(write.eventId == EVENT_2);

    return 0;
}

static int connect_null_source(){
    EventGraphModel model;
    model.setNode(makeNode(NODE_A));
    model.setNode(makeNode(NODE_B));
    model.setPinEventId(ref(NODE_A, 0), EventGraphModel::NULL_EVENT_ID);
    model.setPinEventId(ref(NODE_B, 0), EVENT_2);

    // There's nothing to copy from the source, so copy the other way
    EventWrite write;
    CHECK(model.connect(ref(NODE_A, 0), ref(NODE_B, 0), &write) == ConnectResult::Ok);
    CHECK(write.pin == ref(NODE_A, 0));
    CHECK(write.eventId == EVENT_2);

    return 0;
}

static int connect_rejected(){
    EventGraphModel model;
    model.setNode(makeNode(NODE_A));
    model.setNode(makeNode(NODE_B));
    model.setPinEventId(ref(NODE_A, 0), EventGraphModel::NULL_EVENT_ID);
    model.setPinEventId(ref(NODE_B, 0), EventGraphModel::NULL_EVENT_ID);
    model.setPinEventId(ref(NODE_A, 1), EVENT_1);
    model.setPinEventId(ref(NODE_B, 1), EVENT_1);

    EventWrite write;
    CHECK(model.connect(ref(NODE_A, 0), ref(NODE_B, 0), &write) == ConnectResult::BothEmpty);
    CHECK(model.connect(ref(NODE_A, 1), ref(NODE_B, 1), &write) == ConnectResult::AlreadyConnected);
    CHECK(model.connect(ref(NODE_A, 1), ref(NODE_A, 1), &write) == ConnectResult::SamePin);
    CHECK(model.connect(ref(NODE_A, 1), ref(NODE_B, 2), &write) == ConnectResult::ValueUnknown);
    CHECK(model.connect(ref(NODE_A, 1), ref(NODE_C, 0), &write) == ConnectResult::InvalidPin);

    return 0;
}

static int disconnect(){
    EventGraphModel model;
    model.setNode(makeNode(NODE_A));
    model.setNode(makeNode(NODE_B));
    model.setNode(makeNode(NODE_C));
    model.setPinEventId(ref(NODE_A, 0), EVENT_1);
    model.setPinEventId(ref(NODE_B, 0), EVENT_1);
    model.setPinEventId(ref(NODE_C, 0), EVENT_1);
    model.eventObserved(NODE_A, EVENT_1);

    EventWrite write = model.disconnect(ref(NODE_B, 0));
    CHECK(write.pin == ref(NODE_B, 0));
    CHECK(write.eventId == EventGraphModel::NULL_EVENT_ID);

    // Clearing the producer breaks both links, clearing a consumer only its own
    CHECK(model.linkCount(ref(NODE_A, 0)) == 2);
    CHECK(model.linkCount(ref(NODE_B, 0)) == 1);

    model.setPinEventId(write.pin, write.eventId);
    CHECK(model.links().size() == 1);

    return 0;
}

static int direction_identified(){
    EventGraphModel model;
    model.setNode(makeNode(NODE_A));

    // The identified events can come in before the memory has been read
    model.setIdentifiedEvents(NODE_A, {EVENT_1}, {EVENT_2});
    model.setPinEventId(ref(NODE_A, 0), EVENT_1);
    model.setPinEventId(ref(NODE_A, 1), EVENT_2);
    model.setPinEventId(ref(NODE_A, 2), 0x0501010122000099ull);

    const EventPin* produced = model.pin(ref(NODE_A, 0));
    CHECK(produced->direction == PinDirection::Producer);
    CHECK(produced->directionSource == DirectionSource::Identified);
    CHECK(model.pin(ref(NODE_A, 1))->direction == PinDirection::Consumer);
    // Not listed by the node
    CHECK(model.pin(ref(NODE_A, 2))->direction == PinDirection::Unknown);

    return 0;
}

static int direction_observed_overrides(){
    EventGraphModel model;
    model.setNode(makeNode(NODE_A));
    model.setPinEventId(ref(NODE_A, 0), EVENT_1);

    // The node says that it consumes the event, but we see it send it
    model.setIdentifiedEvents(NODE_A, {}, {EVENT_1});
    CHECK(model.pin(ref(NODE_A, 0))->direction == PinDirection::Consumer);

    QVector<PinRef> sent = model.eventObserved(NODE_A, EVENT_1);
    CHECK(sent.size() == 1);
    CHECK(sent[0] == ref(NODE_A, 0));
    CHECK(model.pin(ref(NODE_A, 0))->direction == PinDirection::Producer);
    CHECK(model.pin(ref(NODE_A, 0))->directionSource == DirectionSource::Observed);

    // Identified events can't undo what we saw
    model.setIdentifiedEvents(NODE_A, {}, {EVENT_1});
    CHECK(model.pin(ref(NODE_A, 0))->direction == PinDirection::Producer);

    // Events that aren't in the node's configuration are ignored
    CHECK(model.eventObserved(NODE_A, EVENT_2).isEmpty());
    CHECK(model.eventObserved(NODE_C, EVENT_1).isEmpty());

    return 0;
}

static int direction_learned(){
    EventGraphModel model;
    model.setNode(makeNode(NODE_A, "Board"));
    model.setPinEventId(ref(NODE_A, 1), EVENT_1);
    model.eventObserved(NODE_A, EVENT_1);

    // Another node of the same model gets the same direction for the same field
    model.setNode(makeNode(NODE_B, "Board"));
    CHECK(model.pin(ref(NODE_B, 1))->direction == PinDirection::Producer);
    CHECK(model.pin(ref(NODE_B, 1))->directionSource == DirectionSource::Learned);
    CHECK(model.pin(ref(NODE_B, 0))->direction == PinDirection::Unknown);

    // ...but not a node of a different model
    model.setNode(makeNode(NODE_C, "Other"));
    CHECK(model.pin(ref(NODE_C, 1))->direction == PinDirection::Unknown);

    // What the node itself says is more reliable than what was learned
    model.setPinEventId(ref(NODE_B, 1), EVENT_2);
    model.setIdentifiedEvents(NODE_B, {}, {EVENT_2});
    CHECK(model.pin(ref(NODE_B, 1))->direction == PinDirection::Consumer);

    // Learned directions can be saved and restored
    EventGraphModel restored;
    restored.setLearnedDirections(model.learnedDirections());
    restored.setNode(makeNode(NODE_A, "Board"));
    CHECK(restored.pin(ref(NODE_A, 1))->directionSource == DirectionSource::Learned);

    return 0;
}

static int storage_round_trip(){
    EventGraphLayout layout;
    layout.positions[NODE_A] = QPointF(10.5, -20);
    layout.positions[NODE_B] = QPointF(300, 400);
    layout.learnedDirections["TrainUtils\nBoard"]["Port I/O/Line 1/Event when on"] = PinDirection::Producer;
    layout.learnedDirections["TrainUtils\nBoard"]["Port I/O/Line 1/Event when off"] = PinDirection::Consumer;

    EventGraphLayout loaded = EventGraphStorage::fromJson(EventGraphStorage::toJson(layout));
    CHECK(loaded.positions.size() == 2);
    CHECK(loaded.positions[NODE_A] == QPointF(10.5, -20));
    CHECK(loaded.positions[NODE_B] == QPointF(300, 400));
    CHECK(loaded.learnedDirections == layout.learnedDirections);

    // Garbage in, empty layout out
    CHECK(EventGraphStorage::fromJson("not json").positions.isEmpty());

    return 0;
}

int main(int argc, char** argv){
    if(argc < 2) return 1;

    struct {
        const char* name;
        int (*fn)();
    } tests[] = {
        {"layout-addresses", layout_addresses},
        {"pins-from-layout", pins_from_layout},
        {"event-id-from-bytes", event_id_from_bytes},
        {"links-producer", links_producer},
        {"links-unknown-direction", links_unknown_direction},
        {"links-ignored", links_ignored},
        {"connect-copies-from-source", connect_copies_from_source},
        {"connect-producer-wins", connect_producer_wins},
        {"connect-null-source", connect_null_source},
        {"connect-rejected", connect_rejected},
        {"disconnect", disconnect},
        {"direction-identified", direction_identified},
        {"direction-observed-overrides", direction_observed_overrides},
        {"direction-learned", direction_learned},
        {"storage-round-trip", storage_round_trip},
    };

    for(const auto& test : tests){
        if(strcmp(argv[1], test.name) == 0){
            return test.fn();
        }
    }

    return 1;
}
