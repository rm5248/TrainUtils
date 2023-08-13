/* SPDX-License-Identifier: GPL-2.0 */
#ifndef LIBLCC_COMMON_INT_H
#define LIBLCC_COMMON_INT_H

#ifndef ARDUINO
#ifndef LIBLCC_BUILD
#error "Internal header, do not use in client code!"
#endif
#endif /* ARDUINO */

#include <stdint.h>

#include "lcc-common.h"

/* This is just an internal file, but arduino seems to use a C++ compiler so we need to make sure functions are C */
#ifdef __cplusplus
extern "C" {
#endif

struct event_list{
#if defined(LIBLCC_EVENT_LIST_STATIC_SIZE)
    uint64_t event_array[LIBLCC_EVENT_LIST_STATIC_SIZE];
#else
    uint64_t* event_array;
#endif
    int size;
    int len;
};

struct lcc_datagram_buffer{
    int offset;
    uint8_t buffer[72];
};

struct lcc_datagram_context{
    struct lcc_context* parent;
    struct lcc_datagram_buffer datagram_buffer;
    lcc_incoming_datagram_fn datagram_received_fn;
    lcc_datagram_received_ok_fn datagram_ok_fn;
    lcc_datagram_rejected_fn datagram_rejected_fn;
};

struct lcc_memory_context{
    struct lcc_context* parent;
    void* cdi_data;
    int cdi_flags;
    int cdi_length;
    lcc_address_space_information_query query_fn;
    lcc_address_space_read read_fn;
    lcc_address_space_write write_fn;
};

struct lcc_event_context{
    struct lcc_context* parent;
    lcc_incoming_event_fn incoming_event;
    struct event_list events_consumed;
    struct event_list events_produced;
    lcc_query_producer_state_fn producer_state_fn;
};

struct lcc_context{
    uint64_t unique_id;
    union{
        uint16_t flags;
        uint16_t reserved : 12, listen_all_events: 1, node_alias_state : 2, state : 1;
    };
    int16_t node_alias;
    lcc_write_fn write_function;
    void* user_data;

    // Simple node information
    struct lcc_simple_node_info simple_info;

    // event producer/consumer
    struct lcc_event_context* event_context;

    // Datagram handling
    struct lcc_datagram_context* datagram_context;

    // Memory handling
    struct lcc_memory_context* memory_context;
};

#define LCC_FLAG_FRAME_ONLY 0
#define LCC_FLAG_FRAME_FIRST 1
#define LCC_FLAG_FRAME_LAST 2
#define LCC_FLAG_FRAME_MIDDLE 3

void lcc_set_lcb_variable_field(struct lcc_can_frame* frame, struct lcc_context* ctx, int variable_field);

void lcc_set_lcb_can_frame_type(struct lcc_can_frame* frame, int type);

void lcc_set_nodeid_in_data(struct lcc_can_frame* frame, uint64_t node_id);

uint64_t lcc_get_node_id_from_data(struct lcc_can_frame* frame);

void lcc_set_flags_and_dest_alias(struct lcc_can_frame* frame, int flag_frame, int alias);

void lcc_set_eventid_in_data(struct lcc_can_frame* frame, uint64_t event_id);

uint64_t lcc_get_eventid_from_data(struct lcc_can_frame* frame);

void event_list_add_event(struct event_list* list, uint64_t event_id);

int event_list_has_event(struct event_list* list, uint64_t event_id);

/**
 * Send out all of the events that we produce.
 *
 * @param ctx
 * @return
 */
int lcc_send_events_produced(struct lcc_context* ctx);

int lcc_handle_datagram(struct lcc_context* ctx, struct lcc_can_frame* frame);

/**
 * Try to handle a datagram with the memory subsystem.
 * Return 1 if it was handled, 0 if it was not.
 */
int lcc_memory_try_handle_datagram(struct lcc_memory_context* ctx, uint16_t alias, uint8_t* data, int data_len);

/**
 * Read a uint32(in big-endian order) from data.  Data must be at least 4 bytes.
 */
uint32_t lcc_uint32_from_data(void* data);

/**
 * Write the uint32 to 'data' in big-endian order
 */
void lcc_uint32_to_data(void* data, uint32_t value);

#ifdef __cplusplus
} /* extern C */
#endif

#endif
