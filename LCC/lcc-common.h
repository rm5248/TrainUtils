/* SPDX-License-Identifier: GPL-2.0 */
#ifndef LIBLCC_COMMON_H
#define LIBLCC_COMMON_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Opaque context used to hold data.
 */
struct lcc_context;

struct lcc_can_frame;

/**
 * A function that will be called in order to write the specified CAN frame out
 * to the bus in an implementation-specific manner
 */
typedef void(*lcc_write_fn)(struct lcc_can_frame*);

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

/*
 * Important bitmasks/fields
 */
#define LCC_FRAME_TYPE_MASK     0x08000000 /* bit 27 */
#define LCC_CAN_FRAME_TYPE_MASK 0x07000000
#define LCC_VARIABLE_FIELD_MASK 0x00FFF000
#define LCC_NID_ALIAS_MASK      0x00000FFF

#define LCC_STATE_INHIBITED 1
#define LCC_STATE_PERMITTED 0

#define LCC_NODE_ALIAS_NOT_SET 0
#define LCC_NODE_ALIAS_SENT_CID 1
#define LCC_NODE_ALIAS_FAIL 2
#define LCC_NODE_ALIAS_GOOD 3

#ifdef __cplusplus
} /* extern C */
#endif

#endif
