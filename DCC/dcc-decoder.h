/* SPDX-License-Identifier: GPL-2.0 */
#ifndef DCC_UTIL_H
#define DCC_UTIL_H

#include <stdint.h>

#define DCC_DECODER_OK  0
#define DCC_DECODER_ERROR_GENERIC -1
#define DCC_DECODER_ERROR_BIT_TIMING -2
#define DCC_DECODER_ERROR_INVALID_PACKET -3
#define DCC_DECODER_ERROR_INVALID_ARG -4

#ifdef __cplusplus
extern "C" {
#endif

struct dcc_decoder;
struct dcc_packet_parser;

typedef void (*dcc_incoming_packet)(struct dcc_decoder* decoder, const uint8_t* packet_bytes, int len);

enum dcc_decoder_direction{
    DCC_DECODER_DIRECTION_FORWARD,
    DCC_DECODER_DIRECTION_REVERSE,
};

enum dcc_decoder_decoding_scheme{
	DCC_DECODER_IRQ_BOTH,
	DCC_DECODER_IRQ_RISING_OR_FALLING,
};

/**
 * Create a new dcc_decoder that will decode signals.
 *
 * @return
 */
struct dcc_decoder* dcc_decoder_new(enum dcc_decoder_decoding_scheme scheme);

void dcc_decoder_free(struct dcc_decoder* );

int dcc_decoder_set_userdata(struct dcc_decoder* decoder, void* user_data);

void* dcc_decoder_userdata(struct dcc_decoder* decoder);

/**
 * When the polarity of the decoded signal changes, call this method.
 * This method may be called from an interrupt handler.
 *
 * This should only be used if you receive an IRQ on both the rising and falling
 * edges of a signal.
 *
 * @param decoder The currently in-use decoder
 * @param timediff The time difference(in microseconds) since the last time that
 * the polarity changed.  If this is the first time the polarity has changed,
 * timediff should be 0.
 */
int dcc_decoder_polarity_changed(struct dcc_decoder* decoder, uint32_t timediff);

/**
 * When the DCC signal rises(or falls), call thsi method.
 * This method may be called from an interrupt handler.
 *
 * This should only be used if you receive an IRQ on either the rising or falling
 * edges of a signal, not if the IRQ happens on both edges.
 *
 * @param decoder
 * @param timediff
 * @return
 */
int dcc_decoder_rising_or_falling(struct dcc_decoder* decoder, uint32_t timediff);

int dcc_decoder_pump_packet(struct dcc_decoder* decoder);

/**
 * Set a packet parser to automatically be used.
 *
 * @param decoder
 * @param parser
 * @return
 */
int dcc_decoder_set_packet_parser(struct dcc_decoder* decoder, struct dcc_packet_parser* parser);

/**
 * Set a callback that will be called whenever a valid packet is decoded.
 *
 * @param decoder
 * @param packet_cb
 * @return
 */
int dcc_decoder_set_packet_callback(struct dcc_decoder* decoder, dcc_incoming_packet packet_cb);


#ifdef __cplusplus
} /* extern C */
#endif

#endif /* DCC_UTIL_H */
