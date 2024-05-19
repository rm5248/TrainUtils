/* SPDX-License-Identifier: GPL-2.0 */
#include <string.h>
#include <stdio.h>

#include "lcc.h"
#include "lcc-common.h"
#include "lcc-event.h"

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

int main(int argc, char** argv){
    if(argc < 2) return 1;

    if(strcmp(argv[1], "decode_2048_1") == 0){
        return decode_2048_1();
    }else if(strcmp(argv[1], "decode_2048_2") == 0){
        return decode_2048_2();
    }

    return 1;
}
