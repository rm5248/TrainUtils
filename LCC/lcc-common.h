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
#define LCC_ERROR_BUFFER_SIZE_INCORRECT -6

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
/** Assumes that our MTI is 16-bits */
#define LCC_MTI_PRIORITY_MASK   (0x3 << 10)
#define LCC_MTI_TYPE_WITHIN_PRIORITY_MASK (0xF << 5)

#define LCC_STATE_INHIBITED 1
#define LCC_STATE_PERMITTED 0

#define LCC_NODE_ALIAS_NOT_SET 0
#define LCC_NODE_ALIAS_SENT_CID 1
#define LCC_NODE_ALIAS_FAIL 2
#define LCC_NODE_ALIAS_GOOD 3

/*
 * Well-known MTI values
 */
#define LCC_MTI_BASIC_INIT_COMPLETE 0x100
#define LCC_MTI_BASIC_VERIFY_NODE_ID_NUM_ADDRESSED 0x488
#define LCC_MTI_BASIC_VERIFY_NODE_ID_NUM_GLOBAL 0x490
#define LCC_MTI_BASIC_VERIFIED_NODE_ID_NUM 0x170
#define LCC_MTI_BASIC_OPTIONAL_INTERACTION_REJECTED 0x068
#define LCC_MTI_BASIC_TERMINATE_DUE_TO_ERROR 0x0A8

#define LCC_MTI_PROTOCOL_SUPPORT_INQUIRE 0x828
#define LCC_MTI_PROTOCOL_SUPPORT_REPLY 0x668

#ifdef __cplusplus
} /* extern C */
#endif

#endif
