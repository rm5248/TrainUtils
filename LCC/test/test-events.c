/* SPDX-License-Identifier: GPL-2.0 */
#include <string.h>
#include <stdio.h>

#include "lcc.h"
#include "lcc-common.h"
#include "lcc-event.h"
#include "lcc-clock.h"

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
    uint64_t event_id = 0x0101020000ff0024; // turnout 15
    struct lcc_accessory_address addr;

    if(lcc_event_id_to_accessory_decoder_2040(event_id, &addr) != LCC_OK){
        return 1;
    }

    if(addr.dcc_accessory_address == 15 && addr.active == 1){
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
    }else if(strcmp(argv[1], "report_time") == 0){
        return report_time();
    }else if(strcmp(argv[1], "encode_2048_1") == 0){
        return encode_2048_1();
    }else if(strcmp(argv[1], "encode_2048_2") == 0){
            return encode_2048_2();
    }

    return 1;
}
