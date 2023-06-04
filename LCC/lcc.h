/* SPDX-License-Identifier: GPL-2.0 */
#ifndef LIBLCC_H
#define LIBLCC_H

#include <stdint.h>

#include "lcc-common.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Create a new LCC context.
 */
struct lcc_context* lcc_context_new(void);

/**
 * Free an LCC context.
 */
void lcc_context_free(struct lcc_context* ctx);

/**
 * Call this method with a CAN frame when data comes in over the CAN bus.
 *
 * @param frame The incoming frame
 * @return LCC status int
 */
int lcc_context_incoming_frame(struct lcc_context* ctx, struct lcc_can_frame* frame);

/**
 * Set a function that will be called in order to write a CAN frame out on the bus
 * @param write_fn
 * @return
 */
int lcc_context_set_write_function(struct lcc_context* ctx, lcc_write_fn write_fn);

/**
 * Set the unique identifier of this node.
 * Note that the unique identifier is 6 bytes; the upper 2 bytes of the parameter are ignored
 *
 * @param ctx
 * @param id
 * @return
 */
int lcc_context_set_unique_identifer(struct lcc_context* ctx, uint64_t id);

/**
 * Get the unique ID of this node.
 *
 * @param ctx
 * @return The unique ID.
 */
uint64_t lcc_context_unique_id(struct lcc_context* ctx);

/**
 * Generate an alias.  Note that the Unique ID and write function must
 * have been set before calling this function.
 *
 * Upon return from this function(if there are no errors), the calling code
 * must wait at least 200ms(as per the spec) and then call lcc_context_claim_alias()
 *
 * @param ctx
 * @return
 */
int lcc_context_generate_alias(struct lcc_context* ctx);

/**
 * Claim the alias that was generated as part of the lcc_context_generate_alias() call.
 *
 * @param ctx
 * @return LCC_OK if the alias did not collide.  If there was a failure, restart
 * the alias negotiation by calling lcc_context_generate_alias()
 */
int lcc_context_claim_alias(struct lcc_context* ctx);

void lcc_context_set_userdata(struct lcc_context* ctx, void* user_data);

/**
 * Get the user data from this context.
 * @param ctx
 * @return
 */
void* lcc_context_user_data(struct lcc_context* ctx);

/**
 * Get the current LCC alias of this context.
 *
 * @param ctx
 * @return
 */
int lcc_context_alias(struct lcc_context* ctx);

/**
 * Set the four main parts of the simple node description, as defined by the simple node information protocol.
 * The data is copied to internal memory.
 *
 * @param ctx
 * @param manufacturer_name
 * @param model_name
 * @param hw_version
 * @param sw_version
 * @return
 */
int lcc_context_set_simple_node_information(struct lcc_context* ctx,
                                            const char* manufacturer_name,
                                            const char* model_name,
                                            const char* hw_version,
                                            const char* sw_version);

/**
 * Set the node name and description, as defined by the simple node information protocol.
 * The data is copied to internal memory.
 *
 * @param ctx
 * @param node_name
 * @param node_description
 * @return
 */
int lcc_context_set_simple_node_name_description(struct lcc_context* ctx,
                                                 const char* node_name,
                                                 const char* node_description);

/**
 * Get the current state(permitted or inhibited)
 *
 * @param ctx
 * @return
 */
int lcc_context_current_state(struct lcc_context* ctx);

/**
 * Get the current datagram context(if it exists).
 *
 * @param ctx
 * @return
 */
struct lcc_datagram_context* lcc_context_get_datagram_context(struct lcc_context* ctx);

/**
 * Get the current memory context(if it exists).
 *
 * @param ctx
 * @return
 */
struct lcc_memory_context* lcc_context_get_memory_context(struct lcc_context* ctx);

/**
 * Get the current event context(if it exists).
 *
 * @param ctx
 * @return
 */
struct lcc_event_context* lcc_context_get_event_context(struct lcc_context* ctx);

#ifdef __cplusplus
} /* extern C */
#endif

#endif
