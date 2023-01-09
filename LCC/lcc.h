/* SPDX-License-Identifier: GPL-2.0 */
#ifndef LIBLCC_RM_H
#define LIBLCC_RM_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Various definitions for LCC */

/*
 * Error code definitions
 */
#define LCC_OK 0
#define LCC_ERROR_GENERIC -1
#define LCC_ERROR_INVALID_ARG -2
#define LCC_ERROR_UNIQUE_ID_NOT_SET -3
#define LCC_ERROR_ALIAS_SET -4
#define LCC_ERROR_ALIAS_FAILURE -5

/**
 * Struct used to pass frames to/from the library.
 * This struct is intentionally pretty much the same as a Linux struct can_frame
 */
struct lcc_can_frame {
        uint32_t can_id;  /* 32 bit CAN_ID + EFF/RTR/ERR flags */
        uint8_t can_len;
        uint8_t res0;
        uint8_t res1;
        uint8_t res2;
        uint8_t data[8];
};

/**
 * Opaque context used to hold data.
 */
struct lcc_context;

/**
 * A function that will be called in order to write the specified CAN frame out
 * to the bus in an implementation-specific manner
 */
typedef void(*lcc_write_fn)(struct lcc_can_frame*);

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

#ifdef __cplusplus
} /* extern C */
#endif

#endif
