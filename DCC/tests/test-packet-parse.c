/* SPDX-License-Identifier: GPL-2.0 */
#include <string.h>
#include <stdio.h>

#include "dcc-decoder.h"
#include "dcc-packet-parser.h"

struct dir_speed{
    enum dcc_decoder_direction dir;
    uint8_t speed;
};

struct accy_cb_info{
    uint16_t accy_number;
    enum dcc_accessory_direction dir;
};

static void dir_speed_cb(struct dcc_packet_parser* decoder, enum dcc_decoder_direction dir, uint8_t speed){
    struct dir_speed* dirspeed = dcc_packet_parser_userdata(decoder);

    dirspeed->dir = dir;
    dirspeed->speed = speed;
}

static void accy_cb(struct dcc_packet_parser* parser, uint16_t accy_number, enum dcc_accessory_direction accy_dir){
    struct accy_cb_info* accy_info = dcc_packet_parser_userdata(parser);

    accy_info->dir = accy_dir;
    accy_info->accy_number = accy_number;
}

static int short_addr1(){
    struct dcc_packet_parser* parser = dcc_packet_parser_new();
    struct dir_speed dspeed = {0};
    const uint8_t data[] = { 0x37, 0x72, 0x45 };

    dcc_packet_parser_set_short_address(parser, 55);
    dcc_packet_parser_set_speed_dir_cb(parser, dir_speed_cb);
    dcc_packet_parser_set_userdata(parser, &dspeed);
    dcc_packet_parser_parse(parser, data, sizeof(data));

    if(dspeed.dir == DCC_DECODER_DIRECTION_FORWARD &&
            dspeed.speed == 2){
        return 0;
    }

    return 1;
}

static int short_addr2(){
    struct dcc_packet_parser* parser = dcc_packet_parser_new();
    struct dir_speed dspeed = {0};
    const uint8_t data[] = { 0x37, 0x52, 0x65 };

    dcc_packet_parser_set_short_address(parser, 55);
    dcc_packet_parser_set_speed_dir_cb(parser, dir_speed_cb);
    dcc_packet_parser_set_userdata(parser, &dspeed);
    dcc_packet_parser_parse(parser, data, sizeof(data));

    if(dspeed.dir == DCC_DECODER_DIRECTION_REVERSE &&
            dspeed.speed == 2){
        return 0;
    }

    return 1;
}

static int accy30_normal(){
    const uint8_t data[] = { 0x88, 0xFB, 0x73 };
    struct dcc_packet_parser* parser = dcc_packet_parser_new();
    struct accy_cb_info cbinfo = {0};

    dcc_packet_parser_set_userdata(parser, &cbinfo);
    dcc_packet_parser_set_accessory_cb(parser, accy_cb);
    dcc_packet_parser_parse(parser, data, sizeof(data));

    if(cbinfo.accy_number == 30 &&
            cbinfo.dir == ACCESSORY_NORMAL){
        return 0;
    }

    return 1;
}

static int accy30_reverse(){
    const uint8_t data[] = { 0x88, 0xFA, 0x72 };
    struct dcc_packet_parser* parser = dcc_packet_parser_new();
    struct accy_cb_info cbinfo = {0};

    dcc_packet_parser_set_userdata(parser, &cbinfo);
    dcc_packet_parser_set_accessory_cb(parser, accy_cb);
    dcc_packet_parser_parse(parser, data, sizeof(data));

    if(cbinfo.accy_number == 30 &&
            cbinfo.dir == ACCESSORY_REVERSE){
        return 0;
    }

    return 1;
}

static int accy1_normal(){
    const uint8_t data[] = { 0x81, 0xF9, 0x78 };
    struct dcc_packet_parser* parser = dcc_packet_parser_new();
    struct accy_cb_info cbinfo = {0};

    dcc_packet_parser_set_userdata(parser, &cbinfo);
    dcc_packet_parser_set_accessory_cb(parser, accy_cb);
    dcc_packet_parser_parse(parser, data, sizeof(data));

    if(cbinfo.accy_number == 1 &&
            cbinfo.dir == ACCESSORY_NORMAL){
        return 0;
    }

    return 1;
}

static int accy1_reverse(){
    const uint8_t data[] = { 0x81, 0xF8, 0x79 };
    struct dcc_packet_parser* parser = dcc_packet_parser_new();
    struct accy_cb_info cbinfo = {0};

    dcc_packet_parser_set_userdata(parser, &cbinfo);
    dcc_packet_parser_set_accessory_cb(parser, accy_cb);
    dcc_packet_parser_parse(parser, data, sizeof(data));

    if(cbinfo.accy_number == 1 &&
            cbinfo.dir == ACCESSORY_REVERSE){
        return 0;
    }

    return 1;
}

int main(int argc, char** argv){
    if(argc < 2) return 1;

    if(strcmp(argv[1], "short-addr1") == 0){
        return short_addr1();
    }else if(strcmp(argv[1], "short-addr2") == 0){
        return short_addr2();
    }else if(strcmp(argv[1], "accy30-normal") == 0){
        return accy30_normal();
    }else if(strcmp(argv[1], "accy30-reverse") == 0){
        return accy30_reverse();
    }else if(strcmp(argv[1], "accy1-normal") == 0){
        return accy1_normal();
    }else if(strcmp(argv[1], "accy1-reverse") == 0){
        return accy1_reverse();
    }

    return 1;
}
