/* SPDX-License-Identifier: GPL-2.0 */
#include <stdlib.h>
#include <string.h>

#include "dcc-decoder.h"

#define ONE_BIT_DURATION_MIN 52
#define ONE_BIT_DURATION_MAX 64
#define ZERO_BIT_DURATION_MIN 90
#define ZERO_BIT_DURATION_MAX 10000

enum PacketParsingState{
    PARSING_PREAMBLE = 0,
    PARSING_DATA_BYTE,
    PARSING_BYTE_START_BIT,
    PARSING_DONE,
};

enum BitTiming{
    ZERO_BIT,
    ONE_BIT,
    INVALID_BIT,
    ZERO_TO_ONE_BIT,
    ONE_TO_ZERO_BIT,
};

struct dcc_decoder{
    uint8_t num_1_bits;
    uint8_t packet_data[12];
    enum PacketParsingState parse_state;
    uint32_t previous_timing;
    uint8_t working_byte;
    uint8_t working_byte_bit;
    uint8_t packet_data_pos;
    uint8_t short_addr;
    void* user_data;
    dcc_decoder_incoming_speed_dir_packet speed_dir;
    dcc_decoder_incoming_estop estop_fn;
};

static enum BitTiming single_timing_to_bit_type(uint32_t timediff){
    if(timediff > ONE_BIT_DURATION_MIN &&
            timediff < ONE_BIT_DURATION_MAX){
        return ONE_BIT;
    }else if(timediff > ZERO_BIT_DURATION_MIN &&
                timediff < ZERO_BIT_DURATION_MAX){
        return ZERO_BIT;
    }

    return INVALID_BIT;
}

static enum BitTiming two_timing_to_bit_type(uint32_t timediff1, uint32_t timediff2){
    enum BitTiming one = single_timing_to_bit_type(timediff1);
    enum BitTiming two = single_timing_to_bit_type(timediff2);

    if(one == ONE_BIT && two == ONE_BIT){
        return ONE_BIT;
    }else if(one == ZERO_BIT && two == ZERO_BIT){
        return ZERO_BIT;
    }else if(one == ONE_BIT && two == ZERO_BIT){
        return ONE_TO_ZERO_BIT;
    }else if(one == ZERO_BIT && two == ONE_BIT){
        return ZERO_TO_ONE_BIT;
    }

    return INVALID_BIT;
}

static int dcc_decoder_is_packet_valid(uint8_t* data, int len){
    // Check the XOR of the packet data, make sure it is good.  If it is not good,
    // we will discard the packet.
    uint8_t xor_data = 0;
    for( int x = 0; x < len - 1; x++ ){
        xor_data ^= data[x];
    }

    if(xor_data == data[len - 1]){
        return 1;
    }

    return 0;
}

static void dcc_decoder_decode_short(struct dcc_decoder* decoder, uint8_t* packet){
    if(packet[0] == 0xFF
            && packet[1] == 0x00
            && packet[2] == 0xFF){
        // Idle packet
        return;
    }

    if(decoder->short_addr != packet[0] && packet[0] != 0){
        return;
    }

    // This is either for us or a broadcast packet, take the appropriate action
    if((packet[1] & 0xC0) == 0x40){
        // speed packet
        int dir = packet[1] & (0x01 << 6);
        int speed = packet[1] & 0x1F;

        if(decoder->speed_dir && speed != 1){
            if(speed != 1){
                speed--;
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

struct dcc_decoder* dcc_decoder_new(void){
    // We're going to assume we can only have one DCC decoder,
    // since there should really be no reason to have more than one.
    static struct dcc_decoder decoder;

    memset(&decoder, 0, sizeof(struct dcc_decoder));

    return &decoder;
}

void dcc_decoder_free(struct dcc_decoder* decoder){
    if(!decoder){
        return;
    }

    memset(decoder, 0, sizeof(struct dcc_decoder));
}

int dcc_decoder_set_userdata(struct dcc_decoder* decoder, void* user_data){
    if(!decoder){
        return DCC_DECODER_ERROR_INVALID_ARG;
    }

    decoder->user_data = user_data;
}

void* dcc_decoder_userdata(struct dcc_decoder* decoder){
    if(!decoder){
        return NULL;
    }

    return decoder->user_data;
}

int dcc_decoder_polarity_changed(struct dcc_decoder* decoder, uint32_t timediff){
    if(timediff == 0){
        // Make sure that we get reset to the initial parsing state
        memset(decoder, 0, sizeof(struct dcc_decoder));
        return DCC_DECODER_OK;
    }

    if(decoder->previous_timing == 0){
        decoder->previous_timing = timediff;
        return DCC_DECODER_OK;
    }

    enum BitTiming timing = two_timing_to_bit_type(decoder->previous_timing, timediff);
    decoder->previous_timing = 0;

    if(timing == INVALID_BIT || timing == ZERO_TO_ONE_BIT || timing == ONE_TO_ZERO_BIT){
        // It looks like we're off for half a bit or something?
        // Don't do any processing on this bit
        return DCC_DECODER_ERROR_BIT_TIMING;
    }

    if(timing == ONE_BIT){
        decoder->num_1_bits++;
    }

    if(timing == ZERO_BIT &&
            decoder->num_1_bits >= 10 &&
            decoder->parse_state == PARSING_PREAMBLE){
        // We have been parsing the preamble, now that we got the 0 bit we can move on
        // to parsing the data from the packet.
        // This zero bit is the packet start bit.
        decoder->parse_state = PARSING_DATA_BYTE;
        decoder->working_byte = 0;
        decoder->working_byte_bit = 7;
        decoder->packet_data_pos = 0;
        return DCC_DECODER_OK;
    }

    if(decoder->parse_state == PARSING_DATA_BYTE){
        if(timing == ONE_BIT){
            decoder->working_byte |= (0x01 << decoder->working_byte_bit);
        }

        if(decoder->working_byte_bit == 0){
            // This is the last bit of the byte, shift it into our buffer
            decoder->packet_data[decoder->packet_data_pos] = decoder->working_byte;
            decoder->packet_data_pos++;
            decoder->working_byte = 0;
            decoder->working_byte_bit = 8;
            decoder->parse_state = PARSING_BYTE_START_BIT;
        }
        decoder->working_byte_bit--;
    }else if(decoder->parse_state == PARSING_BYTE_START_BIT){
        // A zero bit deliminates the bytes on the rails.
        // A one bit means end-of-packet
        if(timing == ZERO_BIT){
            decoder->parse_state = PARSING_DATA_BYTE;
        }else{
            decoder->parse_state = PARSING_DONE;
        }
    }
}

int dcc_decoder_pump_packet(struct dcc_decoder* decoder){
    if(!decoder){
        return DCC_DECODER_ERROR_GENERIC;
    }

    if(decoder->parse_state != PARSING_DONE){
        return DCC_DECODER_OK;
    }

    // Grab the data and let's do something with it.
    uint8_t data[12];
    uint8_t len = decoder->packet_data_pos;
    memcpy(data, decoder->packet_data, sizeof(data));
    decoder->parse_state = PARSING_PREAMBLE;

    // Okay, now let's decode the data
    if(!dcc_decoder_is_packet_valid(data, len)){
        return DCC_DECODER_ERROR_INVALID_PACKET;
    }

    // We have a valid packet at this point, so let's decode it and call the callback(s)
    if(data[0] == 0){
        // Broadcast packet
        dcc_decoder_decode_short(decoder, data);
    }else if(data[0] >= 1 && data[0] <= 127){
        // multi function decoder with 7-bit address
        dcc_decoder_decode_short(decoder, data);
    }

    return DCC_DECODER_OK;
}

int dcc_decoder_set_short_address(struct dcc_decoder* decoder, uint8_t address){
    if(!decoder){
        return DCC_DECODER_ERROR_INVALID_ARG;
    }

    if(address > 128){
        return DCC_DECODER_ERROR_INVALID_ARG;
    }

    decoder->short_addr = address;

    return DCC_DECODER_OK;
}

int dcc_decoder_set_speed_dir_cb(struct dcc_decoder* decoder, dcc_decoder_incoming_speed_dir_packet speed_dir){
    if(!decoder){
        return DCC_DECODER_ERROR_INVALID_ARG;
    }

    decoder->speed_dir = speed_dir;

    return DCC_DECODER_OK;
}
