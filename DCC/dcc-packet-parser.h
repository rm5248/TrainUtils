/* SPDX-License-Identifier: GPL-2.0 */
#ifndef DCCPACKETPARSER_H
#define DCCPACKETPARSER_H

#include <stdint.h>

#include "dcc-decoder.h"

#ifdef __cplusplus
extern "C" {
#endif

enum dcc_accessory_direction{
    ACCESSORY_NORMAL,
    ACCESSORY_REVERSE,
};

typedef void (*dcc_decoder_incoming_speed_dir_packet)(struct dcc_packet_parser* parser, enum dcc_decoder_direction, uint8_t speed);

typedef void (*dcc_decoder_incoming_estop)(struct dcc_packet_parser* parser);

typedef void (*dcc_decoder_incoming_accessory)(struct dcc_packet_parser* parser, uint16_t accy_number, enum dcc_accessory_direction accy_dir);

struct dcc_packet_parser;

struct dcc_packet_parser* dcc_packet_parser_new(void);

void dcc_packet_parser_free(struct dcc_packet_parser* );

int dcc_packet_parser_set_userdata(struct dcc_packet_parser* decoder, void* user_data);

void* dcc_packet_parser_userdata(struct dcc_packet_parser* decoder);

int dcc_packet_parser_parse(struct dcc_packet_parser* decoder, const uint8_t* data, int len);

/**
 * Set the short address of this decoder.  Alternatively, use dcc_decoder_set_long_address if
 * you want to use a long address instead
 *
 * @param decoder
 * @param address
 * @return
 */
int dcc_packet_parser_set_short_address(struct dcc_packet_parser* decoder, uint8_t address);

int dcc_packet_parser_set_speed_dir_cb(struct dcc_packet_parser* decoder, dcc_decoder_incoming_speed_dir_packet speed_dir);

int dcc_packet_parser_set_accessory_cb(struct dcc_packet_parser* parser, dcc_decoder_incoming_accessory incoming_accy);


#ifdef __cplusplus
} /* extern C */
#endif

#endif // DCCPACKETPARSER_H
