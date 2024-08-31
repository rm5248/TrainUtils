/* SPDX-License-Identifier: GPL-2.0 */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

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

struct dcc_decoder* dcc_decoder_new(void){
    struct dcc_decoder* decoder = malloc(sizeof(struct dcc_decoder));
    if(!decoder){
        return NULL;
    }

    memset(decoder, 0, sizeof(struct dcc_decoder));

    return decoder;
}

void dcc_decoder_free(struct dcc_decoder* decoder){
    if(!decoder){
        return;
    }

    free(decoder);
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
        printf("moving to parsing data byte.  num 1 bits: %d\n", decoder->num_1_bits);
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
            printf("got one byte, moving to next.  byte: %d\n", decoder->working_byte);
            // This is the last bit of the byte, shift it into our buffer
            decoder->packet_data[decoder->packet_data_pos] = decoder->working_byte;
            decoder->packet_data_pos++;
            decoder->working_byte = 0;
            decoder->working_byte_bit = 8;
            decoder->parse_state = PARSING_BYTE_START_BIT;
        }
        decoder->working_byte_bit--;
        printf("working byte bit %d = %d\n", decoder->working_byte_bit, timing);
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

    return DCC_DECODER_OK;
}
