/* SPDX-License-Identifier: GPL-2.0 */
#ifndef LIBLCC_EVENT_H
#define LIBLCC_EVENT_H

#include "lcc-common.h"
#include "lcc-clock.h"

struct lcc_event_context;
struct lcc_context;

#define LCC_EVENT_CONTEXT_CLEAR_EVENTS_PRODUCED (0x01 << 0)
#define LCC_EVENT_CONTEXT_CLEAR_EVENTS_CONSUMED (0x01 << 1)

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Struct used to pass around accessory addressing information
 *
 * The DCC accessory address is a 12-bit value, either 0-4088. or 0-2048.
 * See the LCC Event Identifiers Technical Note for why this is the case.
 * Depending on how the event is decoded, the value in this field and the 'active'
 * field can be set differently.
 *
 * Note that
 */
struct lcc_accessory_address {
    uint16_t dcc_accessory_address;
    uint8_t active;
    uint8_t reserved; // must be set to 0
};

struct lcc_event_context* lcc_event_new(struct lcc_context* ctx);

/**
 * Set a function that will be called when an event that we care about comes in.
 *
 * @param ctx The LCC context that received this event
 * @param fn The function to call when an event we care about event comes in
 * @return
 */
int lcc_event_set_incoming_event_function(struct lcc_event_context* ctx,
                                            lcc_incoming_event_fn fn);
/**
 * Add an event to this context that we are interested in.   When the event comes in,
 * the incoming event function will be called.
 *
 * @param ctx The context to add the event to
 * @param event_id The event to listen for.
 * @return
 */
int lcc_event_add_event_consumed(struct lcc_event_context* ctx,
                                   uint64_t event_id);

/**
 * Start a transaction to add many events consumed.
 * @param ctx
 * @return
 */
int lcc_event_add_event_consumed_transaction_start(struct lcc_event_context* ctx);

/**
 * End a transaction that added many events at once.
 * @param ctx
 * @return
 */
int lcc_event_add_event_consumed_transaction_end(struct lcc_event_context* ctx);

/**
 * Remove this event from the context that we are interested in.
 *
 * @param ctx The context to remove the event from
 * @param event_id The event to remove
 * @return
 */
int lcc_event_remove_event_consumed(struct lcc_event_context* ctx,
                                   uint64_t event_id);

/**
 * Clear events from this context
 *
 * @param ctx The context to clear events from
 * @param event_flags Flags determining what events to clear.
 * @return
 */
int lcc_event_clear_events(struct lcc_event_context* ctx, int event_flags);

/**
 * Set a function that will be called in order to determine the current state of an event
 * that is consumed by this node.
 *
 * @param ctx
 * @param consumer_state
 * @return
 */
int lcc_event_add_event_consumed_query_fn(struct lcc_event_context* ctx,
                                            lcc_query_consumer_state_fn consumer_state);

/**
 * Add an event that is logically produced by this node.
 *
 * If you are adding many events, consider using lcc_event_add_event_produced_transaction_start
 * to wrap this function in a transaction when many events are added at once.
 *
 * @param ctx
 * @param event_id The event ID to add
 * @return
 */
int lcc_event_add_event_produced(struct lcc_event_context* ctx,
                                   uint64_t event_id);

/**
 * Start a transaction to add many events produced.
 * @param ctx
 * @return
 */
int lcc_event_add_event_produced_transaction_start(struct lcc_event_context* ctx);

/**
 * End a transaction that added many events at once.
 * @param ctx
 * @return
 */
int lcc_event_add_event_produced_transaction_end(struct lcc_event_context* ctx);

/**
 * Turn on/off the listening of all events.  Note that if all events are listened to,
 * they are not automatically added to the list of events that we respond to when queried.
 *
 * @param ctx
 * @param listen_all
 * @return
 */
int lcc_event_set_listen_all_events(struct lcc_event_context* ctx,
                                      int listen_all);

/**
 * Set a function that will be called in order to determine the current state of an event
 * that is produced by this node.
 *
 * @param ctx
 * @param producer_state
 * @return
 */
int lcc_event_add_event_produced_query_fn(struct lcc_event_context* ctx,
                                            lcc_query_producer_state_fn producer_state);

/**
 * Produce an event.  Note that this event *should* have been added with the
 * lcc_context_add_event_produced method before being called here, but that is not required.
 *
 * @param ctx The context to produce the event on
 * @param event_id The event to emit
 * @return
 */
int lcc_event_produce_event(struct lcc_event_context* ctx,
                              uint64_t event_id);

/**
 * Check to see if the given LCC event ID is for accessory address
 *
 * @param event_id
 * @return 1 if the given event ID is for a accessory address, 0 otherwise
 */
int lcc_event_id_is_accessory_address(uint64_t event_id);

/**
 * Given an event ID, convert it to the LCC accessory address between 1-4088(full range of the DCC accessory decoder range).
 * When decoding using this method, the 'active' field corresponds to bit 3 in the DCC acessory packet(active or inactive).
 *
 * @param event_id
 * @param address Address to fill in
 * @return LCC_OK, or LCC_ERROR_EVENT_NOT_ACCESSORY_DECODER if the event is not an accessory decoder event
 */
int lcc_event_id_to_accessory_decoder(uint64_t event_id, struct lcc_accessory_address* address);

/**
 * Given an event ID, convert it to the LCC accessory address between 1-2040(conventional range of DCC acessory decoders).
 * When decoding using this method, the 'active' field determines if this should be in the normal/closed posistion or the
 * reversed/thrown position.  Bit 3 in the DCC accessory packet is ignored and assumed to always be 1.
 *
 * @param event_id
 * @param address Address to fill in
 * @return LCC_OK, or LCC_ERROR_EVENT_NOT_ACCESSORY_DECODER if the event is not an accessory decoder event
 */
int lcc_event_id_to_accessory_decoder_2040(uint64_t event_id, struct lcc_accessory_address* address);

/**
 * Convert a DCC address to an EventID.  This will correspond to an accessory address between 1-2040.
 *
 * @param address The address to convert
 * @param evnet_id The EventID that corresponds to this address
 * @return LCC_OK if the address is valid and can be converted
 */
int lcc_accessory_decoder_to_event_id_2040(struct lcc_accessory_address* address, uint64_t* event_id);

#ifdef __cplusplus
} /* extern C */
#endif

#endif /* LIBLCC_EVENT_H */
