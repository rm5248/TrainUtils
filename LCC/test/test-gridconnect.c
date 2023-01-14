/* SPDX-License-Identifier: GPL-2.0 */

#include <string.h>

#include "lcc-gridconnect.h"
#include "lcc-common.h"

static int parse1(){
    const char* string = ":X19490481N;";
    struct lcc_can_frame frame;

    if( lcc_gridconnect_to_canframe(string, &frame) < 0 ){
        return 1;
    }

    if(frame.can_len != 0){
        return 1;
    }

    if(frame.can_id == 0x19490481){
        return 0;
    }

    return 1;
}

static int parse2(){
    const char* string = ":X19170481N020112FE57B3;";
    struct lcc_can_frame frame;
    uint8_t expected_bytes[] = { 0x02, 0x01, 0x12, 0xFE, 0x57, 0xB3 };

    if( lcc_gridconnect_to_canframe(string, &frame) < 0 ){
        return 1;
    }

    if(frame.can_len != sizeof(expected_bytes)){
        return 1;
    }

    if(frame.can_id != 0x19170481){
        return 1;
    }

    if(memcmp(expected_bytes, frame.data, sizeof(expected_bytes) ) == 0 ){
        return 0;
    }

    return 1;
}

static int encode1(){
    struct lcc_can_frame frame;
    char buffer[64];
    const char* expected = ":X19490481N;";

    memset(&frame, 0, sizeof(struct lcc_can_frame));
    frame.can_id = 0x19490481;
    frame.can_len = 0;

    int res = lcc_canframe_to_gridconnect(&frame, buffer, sizeof(buffer));
    if(res != 0){
        return 1;
    }

    if(strcmp(buffer, expected) == 0){
        return 0;
    }

    return 1;
}

static int encode2(){
    struct lcc_can_frame frame;
    char buffer[64];
    const char* expected = ":X19170481N020112FE57B3;";

    memset(&frame, 0, sizeof(struct lcc_can_frame));
    frame.can_id = 0x19170481;
    frame.can_len = 6;
    frame.data[0] = 0x02;
    frame.data[1] = 0x01;
    frame.data[2] = 0x12;
    frame.data[3] = 0xFE;
    frame.data[4] = 0x57;
    frame.data[5] = 0xB3;

    int res = lcc_canframe_to_gridconnect(&frame, buffer, sizeof(buffer));
    if(res != 0){
        return 1;
    }

    if(strcmp(buffer, expected) == 0){
        return 0;
    }

    return 1;
}

int main(int argc, char** argv){
    if(argc < 2) return 1;

    if(strcmp(argv[1], "parse1") == 0){
        return parse1();
    }else if(strcmp(argv[1], "parse2") == 0){
        return parse2();
    }else if(strcmp(argv[1], "encode1") == 0){
        return encode1();
    }else if(strcmp(argv[1], "encode2") == 0){
        return encode2();
    }

    return 1;
}
