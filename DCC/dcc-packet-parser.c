/* SPDX-License-Identifier: GPL-2.0 */
#include <stdlib.h>
#include <string.h>

#include "dcc-packet-parser.h"
#include "dcc-decoder.h"

struct dcc_packet_parser{
    uint8_t short_addr;
    void* user_data;

    dcc_decoder_incoming_speed_dir_packet speed_dir;
    dcc_decoder_incoming_estop estop_fn;
};

static void dcc_packet_parser_decode_short(struct dcc_packet_parser* decoder, const uint8_t* packet){
//    if(decoder->short_addr != packet[0] && packet[0] != 0){
//        return;
//    }

    // This is either for us or a broadcast packet, take the appropriate action
    if((packet[1] & 0xC0) == 0x40){
        // speed packet
        int dir = packet[1] & (0x01 << 6);
        int speed = packet[1] & 0x0F;
        int speed_lsb = packet[1] & 0x10;

        speed = speed << 1;
        if(speed_lsb){
            speed |= 0x1;
        }

        if(decoder->speed_dir && speed != 1){
            if(speed != 1){
                speed -= 3;
            }
            decoder->speed_dir(decoder,
                               dir ? DCC_DECODER_DIRECTION_FORWARD : DCC_DECODER_DIRECTION_REVERSE,
                               speed);
        }

        if(decoder->estop_fn && speed == 1){
            decoder->estop_fn(decoder);
        }
    }
}

static void dcc_packet_parser_decode_accessory(struct dcc_packet_parser* decoder, uint8_t* packet){
    uint16_t addr = 0;
    uint16_t msb = 0;

    addr |= (packet[0] & 0x3F);

    // Get the MSB part of the address.  Note that for /some reason/ these
    // bits are in 1-complement.
    msb = (packet[1] & 0x30) >> 4;
    msb = ~msb;
    msb = msb & 0x03;
    msb = msb << 8;
    addr |= msb;

    int activate = packet[1] & (0x01 << 3);
    int data = packet[1] & 0x03;

    activate = !!activate;

    printf("accy decoder addr %d activate %d data %d\n", addr, activate, data);
}

struct dcc_packet_parser* dcc_packet_parser_new(void){
    static struct dcc_packet_parser parser;

    memset(&parser, 0, sizeof(struct dcc_packet_parser));

    return &parser;
}

void dcc_packet_parser_free(struct dcc_packet_parser* parser){
    (void*)parser;
}

int dcc_packet_parser_set_userdata(struct dcc_packet_parser* decoder, void* user_data){
    if(!decoder){
        return DCC_DECODER_ERROR_INVALID_ARG;
    }

    decoder->user_data = user_data;
}

void* dcc_packet_parser_userdata(struct dcc_packet_parser* decoder){
    if(!decoder){
        return NULL;
    }

    return decoder->user_data;
}

int dcc_packet_parser_set_short_address(struct dcc_packet_parser* decoder, uint8_t address){
    if(!decoder){
        return DCC_DECODER_ERROR_INVALID_ARG;
    }

    if(address > 128){
        return DCC_DECODER_ERROR_INVALID_ARG;
    }

    decoder->short_addr = address;

    return DCC_DECODER_OK;
}

int dcc_packet_parser_set_speed_dir_cb(struct dcc_packet_parser* decoder, dcc_decoder_incoming_speed_dir_packet speed_dir){
    if(!decoder){
        return DCC_DECODER_ERROR_INVALID_ARG;
    }

    decoder->speed_dir = speed_dir;

    return DCC_DECODER_OK;
}

int dcc_packet_parser_parse(struct dcc_packet_parser* decoder, const uint8_t* data, int len){
	static int bb = 0;

    if(len == 3
    		&& data[0] == 0xFF
            && data[1] == 0x00
            && data[2] == 0xFF){
        // Idle packet
    	bb++;
        return DCC_DECODER_OK;
    }

//    printf("Packet[%d]: ", bb);
//    for(int x = 0; x < len; x++){
//    	printf("0x%02X ", data[x]);
//    }
//    printf("\n");

    // We have a valid packet at this point, so let's decode it and call the callback(s)
    if(data[0] == 0){
        // Broadcast packet
        dcc_packet_parser_decode_short(decoder, data);
    }else if(data[0] >= 1 && data[0] <= 127){
        // multi function decoder with 7-bit address
        dcc_packet_parser_decode_short(decoder, data);
    }else if(data[0] >= 128 && data[0] <= 191){
        // Accessory decoder range
        dcc_packet_parser_decode_accessory(decoder, data);
    }

    return DCC_DECODER_OK;
}
