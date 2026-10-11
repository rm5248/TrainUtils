/* SPDX-License-Identifier: GPL-2.0 */
#include <string.h>
#include <stdio.h>

#include "lcc.h"
#include "lcc-common.h"
#include "lcc-event.h"
#include "lcc-clock.h"
#include "test-common.h"

static int decode_2048_1(){
    uint64_t event_id = 0x0101020000ff000E; // turnout 4
    struct lcc_accessory_address addr;

    if(lcc_event_id_to_accessory_decoder_2040(event_id, &addr) != LCC_OK){
        return 1;
    }

    if(addr.dcc_accessory_address == 4 && addr.active == 0){
        return 0;
    }
    return 1;
}

static int decode_2048_2(){
    uint64_t event_id = 0x0101020000ff000F; // turnout 4
    struct lcc_accessory_address addr;

    if(lcc_event_id_to_accessory_decoder_2040(event_id, &addr) != LCC_OK){
        return 1;
    }

    if(addr.dcc_accessory_address == 4 && addr.active == 1){
        return 0;
    }
    return 1;
}

static int decode_2048_3(){
    uint64_t event_id = 0x0101020000ff0025; // turnout 15
    struct lcc_accessory_address addr;

    if(lcc_event_id_to_accessory_decoder_2040(event_id, &addr) != LCC_OK){
        return 1;
    }

    if(addr.dcc_accessory_address == 15 && addr.active == 1){
        return 0;
    }
    return 1;
}


static int decode_2048_4(){
    uint64_t event_id = 0x0101020000ff0024; // turnout 15(Reverse/off)
    struct lcc_accessory_address addr;

    if(lcc_event_id_to_accessory_decoder_2040(event_id, &addr) != LCC_OK){
        return 1;
    }

    if(addr.dcc_accessory_address == 15 && addr.active == 0){
        return 0;
    }
    return 1;
}

static int report_time(){
    uint64_t event_id = 0x0101000001000900;
    enum lcc_clock_type type;
    struct lcc_time time;

    int stat = lcc_event_to_report_time_event(event_id, &type, &time);

    if(stat != LCC_OK){
        return 1;
    }

    if(time.hours == 9 && time.minutes == 0 && type == LCC_CLOCK_TYPE_DEFAULT_FAST_CLOCK){
        return 0;
    }

    return 1;
}

static int encode_2048_1(){
    const uint64_t event_id_expected = 0x0101020000ff000E; // turnout 4(Reverse/off)
    uint64_t event_id;
    struct lcc_accessory_address addr;
    addr.dcc_accessory_address = 4;
    addr.active = 0;

    if(lcc_accessory_decoder_to_event_id_2040(&addr, &event_id) != LCC_OK){
        return 1;
    }

    if(event_id == event_id_expected){
        return 0;
    }
    return 1;
}

static int encode_2048_2(){
    const uint64_t event_id_expected = 0x0101020000ff000F; // turnout 4(Normal/on)
    uint64_t event_id;
    struct lcc_accessory_address addr;
    addr.dcc_accessory_address = 4;
    addr.active = 1;

    if(lcc_accessory_decoder_to_event_id_2040(&addr, &event_id) != LCC_OK){
        return 1;
    }

    if(event_id == event_id_expected){
        return 0;
    }
    return 1;
}

/*
 * Events received by node 1 when node 0 sends an event
 */
static int incoming_count;
static uint16_t incoming_alias;
static uint64_t incoming_event_id;

static void incoming_event(struct lcc_context* ctx, uint16_t source_alias, uint64_t event_id){
    incoming_count++;
    incoming_alias = source_alias;
    incoming_event_id = event_id;
}

/*
 * Create two nodes: node 0 sends events, node 1 receives them.
 */
static struct lcc_context** setup_event_nodes(struct lcc_event_context** sender, struct lcc_event_context** receiver){
    struct lcc_context** ctx = lcctest_create_contexts(2);

    *sender = lcc_event_new(ctx[0]);
    *receiver = lcc_event_new(ctx[1]);
    lcc_event_set_incoming_event_function(*receiver, incoming_event);

    incoming_count = 0;
    incoming_alias = 0;
    incoming_event_id = 0;

    return ctx;
}

/*
 * A consumed event is delivered along with the alias of the node that sent it
 */
static int event_source_alias(){
    const uint64_t event_id = 0x0501010122000001ll;
    struct lcc_event_context* sender;
    struct lcc_event_context* receiver;
    struct lcc_context** ctx = setup_event_nodes(&sender, &receiver);

    lcc_event_add_event_consumed(receiver, event_id);

    lcc_event_produce_event(sender, event_id);
    lcctest_pump_frames();

    if(incoming_count != 1){
        fprintf(stderr, "event received %d times\n", incoming_count);
        return 1;
    }
    if(incoming_event_id != event_id){
        fprintf(stderr, "wrong event ID\n");
        return 1;
    }
    if(incoming_alias != lcc_context_alias(ctx[0])){
        fprintf(stderr, "wrong alias: 0x%03X expected 0x%03X\n", incoming_alias, lcc_context_alias(ctx[0]));
        return 1;
    }

    return 0;
}

/*
 * When listening to all events, events are delivered even if they are not consumed
 */
static int event_listen_all(){
    const uint64_t event_id = 0x0501010122000002ll;
    struct lcc_event_context* sender;
    struct lcc_event_context* receiver;
    struct lcc_context** ctx = setup_event_nodes(&sender, &receiver);

    lcc_event_set_listen_all_events(receiver, 1);

    lcc_event_produce_event(sender, event_id);
    lcctest_pump_frames();

    if(incoming_count != 1 ||
            incoming_event_id != event_id ||
            incoming_alias != lcc_context_alias(ctx[0])){
        fprintf(stderr, "event not received correctly(count %d)\n", incoming_count);
        return 1;
    }

    return 0;
}

/*
 * When listening to all events, an event that is also consumed is only delivered once
 */
static int event_listen_all_consumed_once(){
    const uint64_t event_id = 0x0501010122000003ll;
    struct lcc_event_context* sender;
    struct lcc_event_context* receiver;
    setup_event_nodes(&sender, &receiver);

    lcc_event_add_event_consumed(receiver, event_id);
    lcc_event_set_listen_all_events(receiver, 1);

    lcc_event_produce_event(sender, event_id);
    lcctest_pump_frames();

    if(incoming_count != 1){
        fprintf(stderr, "event received %d times\n", incoming_count);
        return 1;
    }

    return 0;
}

/*
 * Events that are not consumed are not delivered when not listening to all events
 */
static int event_not_consumed(){
    struct lcc_event_context* sender;
    struct lcc_event_context* receiver;
    setup_event_nodes(&sender, &receiver);

    lcc_event_add_event_consumed(receiver, 0x0501010122000004ll);

    lcc_event_produce_event(sender, 0x0501010122000005ll);
    lcctest_pump_frames();

    if(incoming_count != 0){
        fprintf(stderr, "event received %d times\n", incoming_count);
        return 1;
    }

    return 0;
}

int main(int argc, char** argv){
    if(argc < 2) return 1;

    if(strcmp(argv[1], "decode_2048_1") == 0){
        return decode_2048_1();
    }else if(strcmp(argv[1], "decode_2048_2") == 0){
        return decode_2048_2();
    }else if(strcmp(argv[1], "decode_2048_3") == 0){
        return decode_2048_3();
    }else if(strcmp(argv[1], "decode_2048_4") == 0){
        return decode_2048_4();
    }else if(strcmp(argv[1], "event-source-alias") == 0){
        return event_source_alias();
    }else if(strcmp(argv[1], "event-listen-all") == 0){
        return event_listen_all();
    }else if(strcmp(argv[1], "event-listen-all-consumed-once") == 0){
        return event_listen_all_consumed_once();
    }else if(strcmp(argv[1], "event-not-consumed") == 0){
        return event_not_consumed();
    }else if(strcmp(argv[1], "report_time") == 0){
        return report_time();
    }else if(strcmp(argv[1], "encode_2048_1") == 0){
        return encode_2048_1();
    }else if(strcmp(argv[1], "encode_2048_2") == 0){
            return encode_2048_2();
    }

    return 1;
}
