/* SPDX-License-Identifier: GPL-2.0 */
#ifndef LCC_SIMPLE_H
#define LCC_SIMPLE_H

#include "lcc-common.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Handle messages with MTIs of:
 * * 0x490 (Verify node ID number global)
 * * 0x8F4
 * * 0x914
 * * 0x970
 * * 0x594
 * * 0x5B4
 *
 * @param ctx
 * @param frame
 * @return
 */
int lcc_handle_simple_protocol(struct lcc_context* ctx, struct lcc_can_frame* frame);

#ifdef __cplusplus
}
#endif

#endif
