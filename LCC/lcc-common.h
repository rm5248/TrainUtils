/* SPDX-License-Identifier: GPL-2.0 */
#ifndef LIBLCC_COMMON_H
#define LIBLCC_COMMON_H

#include <stdint.h>

#include "simplelogger_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Opaque context used to hold data.
 */
struct lcc_context;

/**
 * Opaque context used to hold data for datagrams
 */
struct lcc_datagram_context;

/**
 * Opaque context used to hold information for memory
 */
struct lcc_memory_context;

/**
 * Opaque context used to hold information for remote memory(memory on a node on the bus)
 */
struct lcc_remote_memory_context;

/**
 * Opaque context used to hold information for events
 */
struct lcc_event_context;

struct lcc_can_frame;

enum lcc_producer_state{
    LCC_PRODUCER_VALID,
    LCC_PRODUCER_INVALID,
    LCC_PRODUCER_UNKNOWN
};

/**
 * A function that will be called in order to write the specified CAN frame out
 * to the bus in an implementation-specific manner
 *
 * @return LCC_OK if the frame was able to be sent to the bus(queued up is OK).
 * If there was an error(for example, the queue to send data to the bus is full),
 * this function should return LCC_ERROR_TX.
 */
typedef int(*lcc_write_fn)(struct lcc_context*, struct lcc_can_frame*);

typedef int(*lcc_write_buffer_available)(struct lcc_context*);

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
 * A function that will be called when a datagram is received from a node.
 *
 * Upon a receipt of a datagram, you must call either lcc_datagram_respond_rxok
 * or lcc_datagram_respond_rejected in order to tell the sending node if we
 * accepted or rejected the datagram.
 */
typedef void(*lcc_incoming_datagram_fn)(struct lcc_datagram_context* ctx, uint16_t alias, void* datagram_data, int len);

/**
 * A function that will be called when a node responds with 'datagram received ok'
 */
typedef void (*lcc_datagram_received_ok_fn)(struct lcc_datagram_context* ctx, uint16_t alias, uint8_t flags);

/**
 * A function that will be called when a node responds with 'Datagram Rejected'
 */
typedef void (*lcc_datagram_rejected_fn)(struct lcc_datagram_context* ctx, uint16_t alias, uint16_t error_code, void* optional_data, int optional_len);

/**
 * A function that will be called when a 'Get Address Space Information' command is received
 */
typedef void (*lcc_address_space_information_query)(struct lcc_memory_context* ctx, uint16_t alias, uint8_t address_space);

/**
 * A function that will be called when a Read command is received.
 * The called function must call either lcc_memory_respond_read_reply_ok or
 * lcc_memory_respond_read_reply_fail depending on the result.
 */
typedef void (*lcc_address_space_read)(struct lcc_memory_context* ctx, uint16_t alias, uint8_t address_space, uint32_t starting_address, uint8_t read_count);

/**
 * A function that will be called when a Write command is received.
 * The called function must call either lcc_memory_respond_write_reply_ok or
 * lcc_memory_respond_write_reply_fail depending on the result.
 */
typedef void (*lcc_address_space_write)(struct lcc_memory_context* ctx, uint16_t alias, uint8_t address_space, uint32_t starting_address, void* data, int data_len);

/**
 * A function that will be called in order to trigger the device to reboot
 */
typedef void (*lcc_reboot)(struct lcc_memory_context* ctx);

/**
 * A function that will be called in order to trigger the device to factory reset.
 */
typedef void (*lcc_factory_reset)(struct lcc_memory_context* ctx);

/**
 * A function that will be called after requesting remote memory information.
 * Corresponds to receiving a 'Datagram Received OK' message(Memory Configuration
 * Standard, section 4.4).
 *
 * @see lcc_datagram_received_ok_fn
 */
typedef void (*lcc_remote_memory_request_ok)(struct lcc_remote_memory_context* ctx, uint16_t alias, uint8_t flags);

/**
 * A function that will be called after requesting remote memory information.
 * If the node rejects the datagram, this method will be called.
 *
 * @see lcc_datagram_rejected_fn
 */
typedef void (*lcc_remote_memory_request_fail)(struct lcc_remote_memory_context* ctx, uint16_t alias, uint16_t error_code, void* optional_data, int optional_len);

/**
 * A function that will be called after a valid datagram of memory has been received from a remote node.
 */
typedef void (*lcc_remote_memory_received)(struct lcc_remote_memory_context* ctx, uint16_t alias, uint8_t address_space, uint32_t starting_address, void* memory_data, int len);

/**
 * A function that will be called if:
 *
 * The node rejects the read.  Note that you may first get a 'datagram received OK' response before this is called.
 * This is because the read command consists of two separate datagrams: one to request the data(which LibLCC
 * automatically sends a 'response OK' for with the LCC_DATAGRAM_REPLY_PENDING bit set),
 * and the second datagram that contains the actual data.
 */
typedef void (*lcc_remote_memory_read_rejected)(struct lcc_remote_memory_context* ctx, uint16_t alias, uint8_t address_space, uint32_t starting_address, uint16_t error_code, const char* message);

/*
 * Error code definitions used by the library
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
#define LCC_ERROR_NO_DATAGRAM_HANDLING -9
/** Data was unable to be transmitted.  For example, a queue may be full */
#define LCC_ERROR_TX -10
#define LCC_ERROR_DATAGRAM_IN_PROGRESS -11
#define LCC_ERROR_MEMORY_TX_IN_PROGRESS -12
#define LCC_ERROR_EVENT_NOT_ACCESSORY_DECODER -13

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

#define LCC_ADDRESS_SPACE_PRESENT 0x86
#define LCC_ADDRESS_SPACE_NOT_PRESENT 0x87

#define LCC_DATAGRAM_REPLY_PENDING 0x80

/*
 * LCC protocol error codes
 */
#define LCC_ERRCODE_PERMANENT 0x1000
#define LCC_ERRCODE_SOURCE_NOT_PERMITTED 0x1020
#define LCC_ERRCODE_NOT_IMPLEMENTED 0x1040
#define LCC_ERRCODE_INVALID_ARG 0x1080

#define LCC_ERRCODE_TEMPORARY 0x2000
#define LCC_ERRCODE_TIMEOUT 0x2010
#define LCC_ERRCODE_BUFFER_UNAVAILABLE 0x2020
#define LCC_ERRCODE_NOT_EXPECTED 0x2040
#define LCC_ERRCODE_TRANSFER_ERROR 0x2080

#define LCC_ERRCODE_NOT_IMPLEMENTED_SUBCOMMAND 0x1041
#define LCC_ERRCODE_NOT_IMPLEMENTED_DATAGRAM_STREAM 0x1042
#define LCC_ERRCODE_NOT_IMPLEMENTED_UNKNOWN_MTI 0x1043

#define LCC_ERRCODE_TIMEOUT_END_OF_FRAME 0x2011
#define LCC_ERRCODE_OUT_OF_ORDER_MIDDLE_END 0x2041
#define LCC_ERRCODE_OUT_OF_ORDER_START_BEFORE_FINISH 0x2042

/**
 * Convert a node id to dotted format, putting the result in 'buffer'
 *
 * @param node_id The node ID to convert
 * @param buffer Must be at least 18 bytes long
 * @param buffer_len Must be at least 18 bytes long
 */
int lcc_node_id_to_dotted_format(uint64_t node_id, char* buffer, int buffer_len);

void lcc_set_log_function(simplelogger_log_function log_fn);

#ifdef __cplusplus
} /* extern C */
#endif

#endif
