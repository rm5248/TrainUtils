/* SPDX-License-Identifier: GPL-2.0 */
#ifndef LCC_ADDRESSED_H
#define LCC_ADDRESSED_H

#include "lcc-common.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Handle messages that are addressed to a specific node.
 * If we are not the node they are addressed to, do nothing.
 *
 * @param ctx
 * @param frame
 * @return
 */
int lcc_handle_addressed(struct lcc_context* ctx, struct lcc_can_frame* frame);

/**
 * Handle datagram messages, which are a specific subset of addressed messages.
 *
 * @param ctx
 * @param frame
 * @return
 */
int lcc_handle_datagram(struct lcc_context* ctx, struct lcc_can_frame* frame);

#ifdef __cplusplus
}
#endif

#endif
