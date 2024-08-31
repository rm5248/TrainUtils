/* SPDX-License-Identifier: GPL-2.0 */
#include <string.h>
#include <stdio.h>
#include "dcc-decoder.h"

static void send_preamble(struct dcc_decoder* decoder){
    for( int x = 0; x < 14; x++ ){
        // Send the two half-bits
        dcc_decoder_polarity_changed(decoder, 58);
        dcc_decoder_polarity_changed(decoder, 58);
    }
}

static void send_single_byte(struct dcc_decoder* decoder, uint8_t byte){
    // Send the two half-bits for the 0 bit that indicate this is a data byte
    dcc_decoder_polarity_changed(decoder, 100);
    dcc_decoder_polarity_changed(decoder, 100);

    for(uint8_t byte_pos = 7; byte_pos != 0; byte_pos-- ){
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

static void send_data(struct dcc_decoder* decoder, uint8_t* packet_data, int len){
    for( int x = 0; x < len; x++ ){
        send_single_byte(decoder, packet_data[x]);
    }
}

static void send_full_packet(struct dcc_decoder* decoder, uint8_t* packet_data, int len){
    send_preamble(decoder);
    send_data(decoder, packet_data, len);

    // Now calculate the error byte and send it out
    uint8_t xor_data = 0;
    for(int x = 0; x < len; x++){
        xor_data ^= packet_data[x];
    }
    send_single_byte(decoder, xor_data);

    // Send the terminating bit
    dcc_decoder_polarity_changed(decoder, 58);
    dcc_decoder_polarity_changed(decoder, 58);
}

int basic_packet(){
    struct dcc_decoder* decoder = dcc_decoder_new();

    // Figure 1 of S9.2
    uint8_t packet_data[] = { 55, 116 };
    send_full_packet(decoder, packet_data, sizeof(packet_data));

    dcc_decoder_pump_packet(decoder);
}

int main(int argc, char** argv){
    if(argc < 2) return 1;

    if(strcmp(argv[1], "basic-packet") == 0){
        return basic_packet();
    }

    return 1;
}
