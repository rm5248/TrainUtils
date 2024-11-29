/* SPDX-License-Identifier: GPL-2.0 */
#include <stdlib.h>
#include <string.h>

#include "dcc-decoder.h"
#include "dcc-packet-parser.h"

#define ONE_HALF_BIT_DURATION_MIN 52
#define ONE_HALF_BIT_DURATION_NOM 58
#define ONE_HALF_BIT_DURATION_MAX 64
#define ZERO_HALF_BIT_DURATION_MIN 90
#define ZERO_HALF_BIT_DURATION_NOM 100
#define ZERO_HALF_BIT_DURATION_MAX 10000

#define ONE_BIT_DURATION_MIN (ONE_HALF_BIT_DURATION_MIN * 2)
#define ONE_BIT_DURATION_MAX (ONE_HALF_BIT_DURATION_MAX * 2)
#define ZERO_BIT_DURATION_MIN (ZERO_HALF_BIT_DURATION_MIN * 2)
#define ZERO_BIT_DURATION_MAX (110 * 2)

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
    uint8_t packet_data[6];
    uint8_t last_packet[6];
    enum PacketParsingState parse_state;
    uint32_t previous_timing;
    uint8_t working_byte;
    uint8_t working_byte_bit;
    uint8_t packet_data_pos;
    uint8_t last_packet_valid;
    uint8_t last_packet_len;
    void* user_data;
    enum dcc_decoder_decoding_scheme decoding_scheme;
    struct dcc_packet_parser* packet_parser;
    dcc_incoming_packet packet_cb;
};

static enum BitTiming whole_bit_timing_to_bittiming(uint32_t timediff){
    if(timediff >= ONE_BIT_DURATION_MIN &&
            timediff <= ONE_BIT_DURATION_MAX){
        return ONE_BIT;
    }else if(timediff >= ZERO_BIT_DURATION_MIN){
        return ZERO_BIT;
    }

    return INVALID_BIT;
}

static enum BitTiming single_timing_to_bit_type(uint32_t timediff){
    if(timediff >= ONE_HALF_BIT_DURATION_MIN &&
            timediff <= ONE_HALF_BIT_DURATION_MAX){
        return ONE_BIT;
    }else if(timediff >= ZERO_HALF_BIT_DURATION_MIN &&
                timediff <= ZERO_HALF_BIT_DURATION_MAX){
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

struct dcc_decoder* dcc_decoder_new(enum dcc_decoder_decoding_scheme scheme){
    // We're going to assume we can only have one DCC decoder,
    // since there should really be no reason to have more than one.
    static struct dcc_decoder decoder;

    memset(&decoder, 0, sizeof(struct dcc_decoder));
    decoder.decoding_scheme = scheme;

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

    return DCC_DECODER_OK;
}

void* dcc_decoder_userdata(struct dcc_decoder* decoder){
    if(!decoder){
        return NULL;
    }

    return decoder->user_data;
}

static int dcc_decoder_process_whole_bit(struct dcc_decoder* decoder, enum BitTiming timing){
    if(timing == INVALID_BIT || timing == ZERO_TO_ONE_BIT || timing == ONE_TO_ZERO_BIT){
        // It looks like we're off for half a bit or something?
        // Don't do any processing on this bit
        if(decoder->parse_state == PARSING_PREAMBLE){
            decoder->num_1_bits = 0;
        }
        return DCC_DECODER_ERROR_BIT_TIMING;
    }

    if(timing == ONE_BIT &&
            decoder->parse_state == PARSING_PREAMBLE){
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
    }else if(timing == ZERO_BIT &&
             decoder->num_1_bits < 10 &&
             decoder->parse_state == PARSING_PREAMBLE){
        // We need to re-sync, we got a zero bit before the entire preamble
        decoder->num_1_bits = 0;
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
            if(decoder->packet_data_pos > 5){
                // DCC packets are only 6 bytes.  Reset our parsing.
                decoder->parse_state = PARSING_PREAMBLE;
                decoder->num_1_bits = 0;
            }
        }
        decoder->working_byte_bit--;
    }else if(decoder->parse_state == PARSING_BYTE_START_BIT){
        // A zero bit deliminates the bytes on the rails.
        // A one bit means end-of-packet
        if(timing == ZERO_BIT){
            decoder->parse_state = PARSING_DATA_BYTE;
        }else{
            decoder->parse_state = PARSING_DONE;
            decoder->num_1_bits = 0;
        }
    }

    if(decoder->parse_state == PARSING_DONE){
        memcpy(decoder->last_packet, decoder->packet_data, sizeof(decoder->packet_data));
        memset(decoder->packet_data, 0, sizeof(decoder->packet_data));
        decoder->last_packet_valid = 1;
        decoder->parse_state = PARSING_PREAMBLE;
        decoder->last_packet_len = decoder->packet_data_pos;
        decoder->num_1_bits = 0;
        return 1;
    }

    return DCC_DECODER_OK;
}

int dcc_decoder_polarity_changed(struct dcc_decoder* decoder, uint32_t timediff){
    if(timediff == 0){
        // Make sure that we get reset to the initial parsing state
        decoder->num_1_bits = 0;
        decoder->parse_state = PARSING_PREAMBLE;
        return DCC_DECODER_OK;
    }

    if(decoder->previous_timing == 0){
        decoder->previous_timing = timediff;
        return DCC_DECODER_OK;
    }

    enum BitTiming timing = two_timing_to_bit_type(decoder->previous_timing, timediff);
    if(timing == ZERO_BIT ||
            timing == ONE_BIT){
        // If this is a valid bit, sync up on the next half-bit
        decoder->previous_timing = 0;
    }else{
        // This is an invalid bit or on a transition, set the previous timing so the next
        // half-bit will be synced properly
        decoder->previous_timing = timediff;
    }

    int ret = dcc_decoder_process_whole_bit(decoder, timing);
    return ret;
}

int dcc_decoder_rising_or_falling(struct dcc_decoder* decoder, uint32_t timediff){
    if(timediff == 0){
        // Make sure that we get reset to the initial parsing state
        decoder->num_1_bits = 0;
        decoder->parse_state = PARSING_PREAMBLE;
        return DCC_DECODER_OK;
    }

    enum BitTiming timing = whole_bit_timing_to_bittiming(timediff);
    if(timing == INVALID_BIT){
        // It looks like we're off for half a bit or something?
        // Don't do any processing on this bit
        return DCC_DECODER_ERROR_BIT_TIMING;
    }

    return dcc_decoder_process_whole_bit(decoder, timing);
}

int dcc_decoder_pump_packet(struct dcc_decoder* decoder){
    if(!decoder){
        return DCC_DECODER_ERROR_GENERIC;
    }

    if(!decoder->last_packet_valid){
        return DCC_DECODER_OK;
    }

    decoder->last_packet_valid = 0;

    // Okay, now let's decode the data
    if(!dcc_decoder_is_packet_valid(decoder->last_packet, decoder->last_packet_len)){
//        printf("bad[%d]: ", decoder->last_packet_len);
//        for(int x = 0; x < decoder->last_packet_len; x++){
//            printf("0x%02X,", decoder->last_packet[x]);
//        }
//        printf("\n");
        return DCC_DECODER_ERROR_INVALID_PACKET;
    }

    if(decoder->packet_cb){
        decoder->packet_cb(decoder, decoder->last_packet, decoder->last_packet_len);
    }

    if(decoder->packet_parser){
        return dcc_packet_parser_parse(decoder->packet_parser, decoder->last_packet, decoder->last_packet_len);
    }

    return 1;
}

int dcc_decoder_set_packet_parser(struct dcc_decoder* decoder, struct dcc_packet_parser* parser){
    if(!decoder){
        return DCC_DECODER_ERROR_INVALID_ARG;
    }
    decoder->packet_parser = parser;

    return DCC_DECODER_OK;
}

int dcc_decoder_set_packet_callback(struct dcc_decoder* decoder, dcc_incoming_packet packet_cb){
    if(!decoder){
        return DCC_DECODER_ERROR_INVALID_ARG;
    }
    decoder->packet_cb = packet_cb;

    return DCC_DECODER_OK;
}
