/* SPDX-License-Identifier: GPL-2.0 */
#ifndef DCC_UTIL_H
#define DCC_UTIL_H

#include <stdint.h>

#define DCC_DECODER_OK  0
#define DCC_DECODER_ERROR_GENERIC -1
#define DCC_DECODER_ERROR_BIT_TIMING -2
#define DCC_DECODER_ERROR_INVALID_PACKET -3

#ifdef __cplusplus
extern "C" {
#endif

struct dcc_decoder;

/**
 * Create a new dcc_decoder that will decode signals.
 *
 * @return
 */
struct dcc_decoder* dcc_decoder_new(void);

void dcc_decoder_free(struct dcc_decoder* );

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

#ifdef __cplusplus
} /* extern C */
#endif

#endif /* DCC_UTIL_H */
