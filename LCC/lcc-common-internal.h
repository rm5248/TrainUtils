/* SPDX-License-Identifier: GPL-2.0 */
#ifndef LIBLCC_COMMON_INT_H
#define LIBLCC_COMMON_INT_H

#ifndef ARDUINO
#ifndef LIBLCC_BUILD
#error "Internal header, do not use in client code!"
#endif
#else
#define LIBLCC_ENABLE_STATIC_CONTEXT 1
#endif /* ARDUINO */

#include <stdint.h>

#include "lcc-common.h"

#ifdef ARDUINO_AVR_UNO
#define LCC_SIMPLE_NODE_INFO_SMALL 1
#endif

#ifdef LIBLCC_HAS_CONFIG_H
#include "lcc-config.h"
#endif

#ifdef LIBLCC_ENABLE_STATIC_CONTEXT
#ifndef LIBLCC_EVENT_LIST_STATIC_SIZE
#define LIBLCC_EVENT_LIST_STATIC_SIZE 10
#endif
#endif

#define SIMPLELOGGER_LOG_FUNCTION_NAME lcc_global_log
#include "simplelogger.h"

#ifdef LIBLCC_DEBUG
#define LOG_TRACE(logger, ...) \
do{ char buffer[128]; snprintf(buffer, sizeof(buffer), __VA_ARGS__); SIMPLELOGGER_LOG_CSTR( logger, buffer, SL_TRACE); } while(0)
#define LOG_DEBUG(logger, ...) \
do{ char buffer[128]; snprintf(buffer, sizeof(buffer), __VA_ARGS__); SIMPLELOGGER_LOG_CSTR( logger, buffer, SL_DEBUG); } while(0)
#define LOG_INFO(logger, ...) \
do{ char buffer[128]; snprintf(buffer, sizeof(buffer), __VA_ARGS__); SIMPLELOGGER_LOG_CSTR( logger, buffer, SL_DEBUG); } while(0)
#else
#define LOG_TRACE(logger, ...)
#define LOG_DEBUG(logger, ...)
#define LOG_INFO(logger, ...)
#endif

/* This is just an internal file, but arduino seems to use a C++ compiler so we need to make sure functions are C */
#ifdef __cplusplus
extern "C" {
#endif

extern simplelogger_log_function lcc_global_log;

// Figure out how big our lcc_simple_node_info needs to be.
// Normally, this struct is 251 bytes long.  For memory constrained devices(currently the Uno)
// we set this to a max of 90 bytes.
#ifdef LCC_SIMPLE_NODE_INFO_SMALL
// There is not much RAM on the uno, so we will make this struct much much smaller.
// This is also a common thing that we can define if needed.
struct lcc_simple_node_info {
    char node_information[90];
};
#else
struct lcc_simple_node_info {
    char manufacturer_name[41];
    char model_name[41];
    char hw_version[21];
    char sw_version[21];
    char node_name[63];
    char node_description[64];
};
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
    int currently_handling_incoming_datagram;
};

struct lcc_memory_context{
    struct lcc_context* parent;
    void* cdi_data;
    int cdi_flags;
    int cdi_length;
    lcc_address_space_information_query query_fn;
    lcc_address_space_read read_fn;
    lcc_address_space_write write_fn;
    lcc_reboot reboot_fn;
    lcc_factory_reset factory_reset_fn;
};

struct lcc_remote_memory_context{
    struct lcc_context* parent;
    lcc_remote_memory_request_ok remote_request_ok;
    lcc_remote_memory_request_fail remote_request_fail;
    lcc_remote_memory_received remote_memory_received;
    lcc_remote_memory_read_rejected read_rejected;
    int16_t current_requesting_alias;
};

struct lcc_event_context{
    struct lcc_context* parent;
    lcc_incoming_event_fn incoming_event;
    struct event_list events_consumed;
    struct event_list events_produced;
    lcc_query_producer_state_fn producer_state_fn;
    lcc_query_consumer_state_fn consumer_state_fn;
    uint8_t in_add_produced_event_transaction : 1;
    uint8_t in_add_consumed_event_transaction : 1;
};

struct lcc_firmware_upgrade_context{
    struct lcc_context* parent;
    uint8_t upgrade_in_progress;
    uint8_t calling_write_cb;
    lcc_firmware_upgrade_start start_fn;
    lcc_firmware_upgrade_incoming_data write_fn;
    lcc_firmware_upgrade_finished finished_fn;
    uint16_t alias;
    uint32_t addr;
};

struct lcc_context{
    uint64_t unique_id;
    union{
        uint16_t flags;
        struct{
            uint16_t listen_all_events: 1;
            uint16_t node_alias_state : 2;
            uint16_t state : 1;
        };
    };
    int16_t node_alias;
    lcc_write_fn write_function;
    lcc_write_buffer_available write_buffer_avail_function;
    int write_buffer_size;
    void* user_data;

    // Simple node information
    struct lcc_simple_node_info simple_info;

    // event producer/consumer
    struct lcc_event_context* event_context;

    // Datagram handling
    struct lcc_datagram_context* datagram_context;

    // Memory handling
    struct lcc_memory_context* memory_context;

    // Remote memory handling
    struct lcc_remote_memory_context* remote_memory_context;

    // Firmware upgrade handling
    struct lcc_firmware_upgrade_context* firmware_upgrade_context;
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

void event_list_add_event(struct event_list* list, uint64_t event_id, int sort);

void event_list_remove_event(struct event_list* list, uint64_t event_id);

void event_list_clear(struct event_list* list);

int event_list_has_event(struct event_list* list, uint64_t event_id);

/**
 * Sort the event list.  Required for the bsearch to work properly.
 * @param list
 */
void event_list_sort(struct event_list* list);

/**
 * Send out all of the events that we produce.  Internal method, do not call.
 *
 * @param ctx
 * @return
 */
int lcc_send_events_produced(struct lcc_context* ctx);

/**
 * Send out all of the enevts that we consume.  Internal method, do not call.
 *
 * @param ctx
 * @return
 */
int lcc_send_events_consumed(struct lcc_context* ctx);

int lcc_handle_datagram(struct lcc_context* ctx, struct lcc_can_frame* frame);

/**
 * Try to handle a datagram with the memory subsystem.
 * Return 1 if it was handled, 0 if it was not.
 */
int lcc_memory_try_handle_datagram(struct lcc_memory_context* ctx, uint16_t alias, uint8_t* data, int data_len);

/**
 * Try to handle a datagram with the remote memory subsystem.
 * Return 1 if it was handled, 0 if it was not.
 */
int lcc_remote_memory_try_handle_datagram(struct lcc_remote_memory_context* ctx, uint16_t alias, uint8_t* data, int data_len);
int lcc_remote_memory_handle_datagram_rx_ok(struct lcc_remote_memory_context* ctx, uint16_t alias, uint8_t flags);
int lcc_remote_memory_handle_datagram_rejected(struct lcc_remote_memory_context* ctx, uint16_t alias, uint16_t error_code, void* optional_data, int optional_len);

int _lcc_firmware_upgrade_freeze(struct lcc_firmware_upgrade_context* ctx, uint16_t alias);
int _lcc_firmware_upgrade_incoming_write(struct lcc_firmware_upgrade_context* ctx, uint16_t alias, uint32_t starting_address, uint8_t* data, int data_len);
int _lcc_firmware_upgrade_unfreeze(struct lcc_firmware_upgrade_context* ctx, uint16_t alias);

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
