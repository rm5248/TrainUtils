/* SPDX-License-Identifier: GPL-2.0 */
#ifndef LIBLCC_GRIDCONNECT_H
#define LIBLCC_GRIDCONNECT_H

#include <stdint.h>

#include "lcc-common.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Opaque struct that handles data associated with an adapter using the gridconnect ASCII protocol.
 */
struct lcc_gridconnect;

struct lcc_gridconnect* lcc_gridconnect_new(void);

void lcc_gridconnect_free(struct lcc_gridconnect*);

int lcc_gridconnect_incoming_data(struct lcc_gridconnect* context, void* data, uint32_t len);

/**
 * Convert an LCC can frame to gridconnect ASCII format.
 *
 * @param frame The frame to convert
 * @param output Where to put the ASCII data
 * @param out_len How long the buffer is
 * @return LCC_OK on success, error code otherwise.
 */
int lcc_canframe_to_gridconnect(struct lcc_can_frame* frame, char* output, int out_len);

/**
 * Convert a Gridconnect ASCII dump to an LCC frame.
 *
 * @param ascii A complete frame in gridconnect protocol.
 * @param frame The resulting frame
 * @return LCC_OK on success, error code otherwise.
 */
int lcc_gridconnect_to_canframe(char* ascii, struct lcc_can_frame* frame);

#ifdef __cplusplus
} /* extern C */
#endif

#endif /* LIBLCC_GRIDCONNECT_H */
