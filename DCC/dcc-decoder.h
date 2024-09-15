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

enum dcc_decoder_direction{
    DCC_DECODER_DIRECTION_FORWARD,
    DCC_DECODER_DIRECTION_REVERSE,
};

enum dcc_decoder_decoding_scheme{
	DCC_DECODER_IRQ_BOTH,
	DCC_DECODER_IRQ_RISING_OR_FALLING,
};

typedef void (*dcc_decoder_incoming_speed_dir_packet)(struct dcc_decoder* decoder, enum dcc_decoder_direction, uint8_t speed);

typedef void (*dcc_decoder_incoming_estop)(struct dcc_decoder* decoder);

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
 * @param decoder The currently in-use decoder
 * @param timediff The time difference(in microseconds) since the last time that
 * the polarity changed.  If this is the first time the polarity has changed,
 * timediff should be 0.
 */
int dcc_decoder_polarity_changed(struct dcc_decoder* decoder, uint32_t timediff);

int dcc_decoder_pump_packet(struct dcc_decoder* decoder);

/**
 * Set the short address of this decoder.  Alternatively, use dcc_decoder_set_long_address if
 * you want to use a long address instead
 *
 * @param decoder
 * @param address
 * @return
 */
int dcc_decoder_set_short_address(struct dcc_decoder* decoder, uint8_t address);

int dcc_decoder_set_speed_dir_cb(struct dcc_decoder* decoder, dcc_decoder_incoming_speed_dir_packet speed_dir);

#ifdef __cplusplus
} /* extern C */
#endif

#endif /* DCC_UTIL_H */
