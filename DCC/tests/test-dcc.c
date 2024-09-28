/* SPDX-License-Identifier: GPL-2.0 */
#include <string.h>
#include <stdio.h>

#include "dcc-decoder.h"
#include "dcc-packet-parser.h"

static void send_preamble_fullbit(struct dcc_decoder* decoder){
    for( int x = 0; x < 14; x++ ){
        dcc_decoder_rising_or_falling(decoder, 58 * 2);
    }
}

static void send_single_byte_fullbit(struct dcc_decoder* decoder, uint8_t byte){
    dcc_decoder_rising_or_falling(decoder, 100 * 2);

    for(int byte_pos = 7; byte_pos >= 0; byte_pos-- ){
        int duration = 0;
        if( byte & (0x01 << byte_pos) ){
            duration = 58;
            printf("send 1 bit\n");
        }else{
            printf("send 0 bit\n");
            duration = 100;
        }

        dcc_decoder_rising_or_falling(decoder, duration * 2);
    }
}

static void send_data_fullbit(struct dcc_decoder* decoder, uint8_t* packet_data, int len){
    for( int x = 0; x < len; x++ ){
        send_single_byte_fullbit(decoder, packet_data[x]);
    }
}

static void send_full_packet_fullbits(struct dcc_decoder* decoder, uint8_t* packet_data, int len){
    send_preamble_fullbit(decoder);
    send_data_fullbit(decoder, packet_data, len);

    // Now calculate the error byte and send it out
    uint8_t xor_data = 0;
    for(int x = 0; x < len; x++){
        xor_data ^= packet_data[x];
    }
    send_single_byte_fullbit(decoder, xor_data);

    // Send the terminating bit
    dcc_decoder_rising_or_falling(decoder, 58 * 2);
}

// ------------------ halfbit methods
static void send_preamble_halfbit(struct dcc_decoder* decoder){
    for( int x = 0; x < 14; x++ ){
        // Send the two half-bits
        dcc_decoder_polarity_changed(decoder, 58);
        dcc_decoder_polarity_changed(decoder, 58);
    }
}

static void send_single_byte_halfbit(struct dcc_decoder* decoder, uint8_t byte){
    // Send the two half-bits for the 0 bit that indicate this is a data byte
    dcc_decoder_polarity_changed(decoder, 100);
    dcc_decoder_polarity_changed(decoder, 100);

    for(int byte_pos = 7; byte_pos >= 0; byte_pos-- ){
        int duration = 0;
        if( byte & (0x01 << byte_pos) ){
            duration = 58;
            printf("send 1 bit\n");
        }else{
            printf("send 0 bit\n");
            duration = 100;
        }

        dcc_decoder_polarity_changed(decoder, duration);
        dcc_decoder_polarity_changed(decoder, duration);
    }
}

static void send_data_halfbit(struct dcc_decoder* decoder, uint8_t* packet_data, int len){
    for( int x = 0; x < len; x++ ){
        send_single_byte_halfbit(decoder, packet_data[x]);
    }
}

static void send_full_packet_halfbits(struct dcc_decoder* decoder, uint8_t* packet_data, int len){
    send_preamble_halfbit(decoder);
    send_data_halfbit(decoder, packet_data, len);

    // Now calculate the error byte and send it out
    uint8_t xor_data = 0;
    for(int x = 0; x < len; x++){
        xor_data ^= packet_data[x];
    }
    send_single_byte_halfbit(decoder, xor_data);

    // Send the terminating bit
    dcc_decoder_polarity_changed(decoder, 58);
    dcc_decoder_polarity_changed(decoder, 58);
}

struct speeddir{
    enum dcc_decoder_direction dir;
    uint8_t speed;
};

static void speed_dir_cb(struct dcc_packet_parser* decoder, enum dcc_decoder_direction dir, uint8_t speed){
    struct speeddir* sp = dcc_packet_parser_userdata(decoder);
    sp->dir = dir;
    sp->speed = speed;
}

int basic_packet_halfbit(){
    struct dcc_decoder* decoder = dcc_decoder_new(DCC_DECODER_IRQ_BOTH);
    struct speeddir sp = {0};
    struct dcc_packet_parser* parser = dcc_packet_parser_new();

    dcc_decoder_set_packet_parser(decoder, parser);

    dcc_packet_parser_set_speed_dir_cb(parser, speed_dir_cb);
    dcc_packet_parser_set_userdata(parser, &sp);
    dcc_packet_parser_set_short_address(parser, 55);

    // Figure 1 of S9.2
    uint8_t packet_data[] = { 55, 116 };
    send_full_packet_halfbits(decoder, packet_data, sizeof(packet_data));

    dcc_decoder_pump_packet(decoder);

    if(sp.dir == DCC_DECODER_DIRECTION_FORWARD && sp.speed == 6){
        return 0;
    }

    return 1;
}

int basic_packet_fullbit(){
    struct dcc_decoder* decoder = dcc_decoder_new(DCC_DECODER_IRQ_RISING_OR_FALLING);
    struct speeddir sp = {0};
    struct dcc_packet_parser* parser = dcc_packet_parser_new();

    dcc_decoder_set_packet_parser(decoder, parser);

    dcc_packet_parser_set_speed_dir_cb(parser, speed_dir_cb);
    dcc_packet_parser_set_userdata(parser, &sp);
    dcc_packet_parser_set_short_address(parser, 55);

    // Figure 1 of S9.2
    uint8_t packet_data[] = { 55, 116 };
    send_full_packet_fullbits(decoder, packet_data, sizeof(packet_data));

    dcc_decoder_pump_packet(decoder);

    if(sp.dir == DCC_DECODER_DIRECTION_FORWARD && sp.speed == 6){
        return 0;
    }

    return 1;
}

int multiple_fullbit_packets(){
    uint8_t data[] = {
    36,114,107,116,116,123,109,116,116,126,107,115,116,126,107,167,
    168,116,127,108,115,116,116,123,161,219,219,219,219,219,219,219,
    219,224,163,127,105,125,108,116,116,125,108,116,116,117,124,108,
    116,116,126,108,114,116,116,123,110,116,116,122,162,168,116,175,
    161,116,121,163,219,219,170,114,116,116,124,109,116,175,219,213,
    168,116,168,219,219,221,220,225,221,157,174,218,226,207,168,178,
    158,116,127,109,113,116,116,125,109,115,116,116,116,120,112,116,
    128,106,166,168,116,116,120,113,116,116,125,159,219,219,219,219,
    220,221,225,220,210,168,116,116,116,127,108,113,116,116,123,109,
    116,116,116,122,110,116,116,125,109,115,116,116,128,107,114,179,
    157,116,125,108,116,116,126,108,167,219,219,219,219,219,220,221,
    225,220,157,116,116,116,128,107,114,116,116,128,107,113,116,116,
    122,111,116,116,116,123,110,116,116,128,107,113,168,168,116,169,
    166,125,109,166,219,219,176,108,116,116,116,119,113,174,214,221,
    166,120,164,232,207,230,208,229,221,157,168,219,219,219,168,168,
    168,116,116,119,114,116,122,111,116,116,128,105,116,116,118,114,
    116,125,159,168,116,126,107,116,116,120,112,174,219,214,219,219,
    219,219,219,219,219,168,116,116,116,123,109,116,116,120,112,116,
    123,109,116,116,123,110,116,127,106,115,116,125,108,116,127,157,
    168,116,116,121,112,116,116,120,164,231,207,231,208,231,208,219,
    219,219,168,116,116,126,107,116,116,122,110,116,125,107,116,116,
    122,110,116,125,107,116,116,123,110,116,126,107,168,168,116,168,
    171,113,116,168,219,219,170,114,116,123,109,116,116,168,219,219,
    173,111,176,220,218,223,217,222,217,158,168,219,219,219,174,162,
    168,116,127,107,115,116,116,120,112,116,116,125,108,116,116,126,
    107,116,173,163,124,108,116,116,120,112,116,168,219,219,221,219,
    222,217,221,217,221,165,124,108,121,112,116,127,107,114,116,116,
    118,115,116,119,113,116,126,107,116,128,105,116,116,124,108,177,
    158,126,106,116,116,119,114,116,168,219,219,219,219,219,219,219,
    219,219,168,116,116,116,125,107,116,116,125,108,116,116,120,113,
    116,116,120,112,116,127,107,115,116,116,118,114,172,164,121,163,
    168,116,124,160,219,219,168,116,116,121,112,116,128,156,219,219,
    170,114,168,228,221,216,221,217,221,160,168,219,219,219,168,168,
    168,116,116,116,116,124,108,116,116,123,109,116,116,126,107,116,
    116,120,164,168,116,123,110,116,116,123,110,178,215,221,217,221,
    216,222,218,222,218,161,127,107,116,116,127,107,115,116,123,109,
    116,116,127,107,115,116,125,107,116,116,128,106,114,116,126,158,
    173,111,116,127,107,115,116,116,168,219,219,219,219,219,219,219,
    219,219,170,115,116,116,128,106,115,116,116,121,111,116,116,120,
    112,116,116,122,111,116,116,119,113,116,116,122,162,168,116,168,
    168,119,114,176,221,219,158,116,125,108,116,116,117,167,219,219,
    168,116,168,219,219,219,219,219,219,172,164,219,219,219,168,170,
    165,121,111,116,116,123,109,116,116,127,106,116,116,125,107,116,
    116,122,162,168,116,125,107,116,116,125,107,168,230,209,231,208,
    231,208,228,221,217,160,128,105,122,110,116,116,122,111,116,128,
    106,115,116,127,107,115,116,124,108,116,116,126,107,115,116,168,
    175,109,116,116,119,114,116,116,168,219,219,219,219,219,219,219,
    220,224,162,128,106,122,108,116,116,121,112,116,116,121,112,116,
    125,108,116,116,124,108,116,116,119,114,116,116,168,168,116,168,
    168,118,114,174,221,217,162,125,108,116,116,127,107,166,219,219,
    168,116,168,219,219,220,219,223,219,164,168,219,219,219,178,157,
    169,116,116,122,110,116,116,128,106,116,116,116,123,109,116,116,
    126,107,167,168,116,127,108,115,116,126,107,167,219,219,219,219,
    219,219,219,219,219,178,106,123,109,116,116,128,107,115,116,127,
    108,114,116,116,123,110,116,127,106,115,116,127,106,116,116,168,
    174,110,116,127,107,114,116,116,168,219,219,219,219,219,219,219,
    219,219,177,107,121,112,116,127,107,114,116,116,122,111,116,116,
    120,113,116,116,123,110,116,116,124,109,116,116,168,177,107,180,
    156,116,121,163,219,219,168,116,116,125,108,116,116,168,221,217,
    168,120,164,219,219,219,219,219,219,169,166,231,208,219,168,177,
    158,116,121,112,116,116,122,111,116,116,116,126,107,115,116,127,
    108,114,174,162,124,109,116,116,124,109,116,169,218,220,219,221,
    218,222,217,223,219,162,128};
    struct dcc_decoder* decoder = dcc_decoder_new(DCC_DECODER_IRQ_RISING_OR_FALLING);

    for( int x = 0; x < (sizeof(data) / sizeof(data[0])); x++ ){
        dcc_decoder_rising_or_falling(decoder, data[x]);
        dcc_decoder_pump_packet(decoder);
    }
}

int main(int argc, char** argv){
    if(argc < 2) return 1;

    if(strcmp(argv[1], "basic-packet-halfbit") == 0){
        return basic_packet_halfbit();
    }else if(strcmp(argv[1], "basic-packet-fullbit") == 0){
        return basic_packet_fullbit();
    }else if(strcmp(argv[1], "fullbit-packets") == 0){
        return multiple_fullbit_packets();
    }

    return 1;
}
