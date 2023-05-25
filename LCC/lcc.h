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
 * Set a function that will be called when an event that we care about comes in.
 *
 * @param ctx The LCC context that received this event
 * @param fn The function to call when an event we care about event comes in
 * @return
 */
int lcc_context_set_incoming_event_function(struct lcc_context* ctx,
                                            lcc_incoming_event_fn fn);
/**
 * Add an event to this context that we are interested in.   When the event comes in,
 * the incoming event function will be called.
 *
 * @param ctx The context to add the event to
 * @param event_id The event to listen for.
 * @return
 */
int lcc_context_add_event_consumed(struct lcc_context* ctx,
                                   uint64_t event_id);

/**
 * Add an event that is logically produced by this node.
 *
 * @param ctx
 * @param event_id The event ID to add
 * @return
 */
int lcc_context_add_event_produced(struct lcc_context* ctx,
                                   uint64_t event_id);

/**
 * Turn on/off the listening of all events.  Note that if all events are listened to,
 * they are not automatically added to the list of events that we respond to when queried.
 *
 * @param ctx
 * @param listen_all
 * @return
 */
int lcc_context_set_listen_all_events(struct lcc_context* ctx,
                                      int listen_all);

/**
 * Set a function that will be called in order to determine the current state of an event
 * that is produced by this node.
 *
 * @param ctx
 * @param producer_state
 * @return
 */
int lcc_context_add_event_produced_query_fn(struct lcc_context* ctx,
                                            lcc_query_producer_state_fn producer_state);

/**
 * Produce an event.  Note that this event *should* have been added with the
 * lcc_context_add_event_produced method before being called here, but that is not required.
 *
 * @param ctx The context to produce the event on
 * @param event_id The event to emit
 * @return
 */
int lcc_context_produce_event(struct lcc_context* ctx,
                              uint64_t event_id);

/**
 * Set a function that will be called during datagram transfers.
 * This needs to be setup before doing something that requires datagrams, for example
 * reading the CDI or memory.
 *
 * @param ctx
 * @param incoming_datagram
 * @return
 */
int lcc_context_set_datagram_functions(struct lcc_context* ctx,
                                       lcc_incoming_datagram_fn incoming_datagram,
                                       lcc_datagram_received_ok_fn datagram_ok,
                                       lcc_datagram_rejected_fn datagram_rejected);

/**
 * Get the current state(permitted or inhibited)
 *
 * @param ctx
 * @return
 */
int lcc_context_current_state(struct lcc_context* ctx);

#ifdef __cplusplus
} /* extern C */
#endif

#endif
