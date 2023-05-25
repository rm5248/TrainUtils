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

enum lcc_producer_state{
    LCC_PRODUCER_VALID,
    LCC_PRODUCER_INVALID,
    LCC_PRODUCER_UNKNOWN
};

/**
 * A function that will be called in order to write the specified CAN frame out
 * to the bus in an implementation-specific manner
 */
typedef void(*lcc_write_fn)(struct lcc_context*, struct lcc_can_frame*);

/**
 * A function that will be called when an event that we are interested in comes in.
 */
typedef void(*lcc_incoming_event_fn)(struct lcc_context* ctx, uint64_t event_id);

/**
 * A function that will be called when producers are being queried.  For the given event,
 * give back an enum LCC_PRODUCER_<state> determinging what the current state of the event is.
 *
 * This is for the Identify Producer message, MTI 0x0914
 */
typedef enum lcc_producer_state(*lcc_query_producer_state_fn)(struct lcc_context* ctx, uint64_t event_id);

/**
 * A function that will be called when a datagram is received from a node
 */
typedef void(*lcc_incoming_datagram_fn)(struct lcc_context* ctx, void* datagram_data, int len);

/**
 * A function that will be called when a node responds with 'datagram received ok'
 */
typedef void (*lcc_datagram_received_ok_fn)(struct lcc_context* ctx, uint8_t flags);

/**
 * A function that will be called when a node responds with 'Datagram Rejected'
 */
typedef void (*lcc_datagram_rejected_fn)(struct lcc_context* ctx, uint16_t error_code, void* optional_data, int optional_len);

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
/** A string provided to a function is too long(see spec for more details) */
#define LCC_ERROR_STRING_TOO_LONG -7
#define LCC_ERROR_EVENT_ID_INVALID -8

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

struct lcc_simple_node_info {
    char manufacturer_name[41];
    char model_name[41];
    char hw_version[21];
    char sw_version[21];
    char node_name[63];
    char node_description[64];
};

/*
 * Important bitmasks/fields
 */
#define LCC_FRAME_TYPE_MASK     0x08000000 /* bit 27 */
#define LCC_CAN_FRAME_TYPE_MASK 0x07000000
#define LCC_VARIABLE_FIELD_MASK 0x00FFF000
#define LCC_NID_ALIAS_MASK      0x00000FFF
#define LCC_CAN_FRAME_ID_MASK   0x1FFFFFFF
/** Assumes that our MTI is 16-bits */
#define LCC_MTI_PRIORITY_MASK   (0x3 << 10)
#define LCC_MTI_TYPE_WITHIN_PRIORITY_MASK (0xF << 5)
#define LCC_MTI_SIMPLE (0x01 << 4)
#define LCC_MTI_ADDRESSED (0x01 << 3)
#define LCC_MTI_EVENT_NUM_PRESENT (0x01 << 2)
#define LCC_MTI_STREAM_OR_DATAGRAM (0x01 << 12)

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

#define LCC_MTI_SIMPLE_NODE_INFORMATION_REQUEST 0x0DE8
#define LCC_MTI_SIMPLE_NODE_INFORMATION_REPLY 0x0A08

#define LCC_MTI_EVENT_IDENTIFY_CONSUMER 0x08F4
#define LCC_MTI_EVENT_IDENTIFY_PRODUCER 0x0914
#define LCC_MTI_EVENT_IDENTIFY_EVENTS_GLOBAL 0x0970
#define LCC_MTI_EVENT_IDENTIFY 0x0968
#define LCC_MTI_EVENT_LEARN_EVENT 0x594
#define LCC_MTI_PRODUCER_CONSUMER_EVENT_REPORT 0x05B4

#define LCC_MTI_CONSUMER_IDENTIFIED_VALID 0x04C4
#define LCC_MTI_CONSUMER_IDENTIFIED_INVALID 0x04C5
#define LCC_MTI_CONSUMER_IDENTIFIED_UNKNOWN 0x04C7

#define LCC_MTI_PRODUCER_IDENTIFIED_VALID 0x0544
#define LCC_MTI_PRODUCER_IDENTIFIED_INVALID 0x0545
#define LCC_MTI_PRODUCER_IDENTIFIED_UNKNOWN 0x0547

#define LCC_MTI_DATAGRAM_RECEIVED_OK 0xA28
#define LCC_MTI_DATAGRAM_REJECTED 0xA48

/**
 * Convert a node id to dotted format, putting the result in 'buffer'
 *
 * @param node_id The node ID to convert
 * @param buffer Must be at least 18 bytes long
 * @param buffer_len Must be at least 18 bytes long
 */
int lcc_node_id_to_dotted_format(uint64_t node_id, char* buffer, int buffer_len);

#ifdef __cplusplus
} /* extern C */
#endif

#endif
