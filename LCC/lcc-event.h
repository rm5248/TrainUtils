/* SPDX-License-Identifier: GPL-2.0 */
#ifndef LIBLCC_EVENT_H
#define LIBLCC_EVENT_H

#include "lcc-common.h"

struct lcc_event_context;
struct lcc_context;

#ifdef __cplusplus
extern "C" {
#endif

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
 * Add an event that is logically produced by this node.
 *
 * @param ctx
 * @param event_id The event ID to add
 * @return
 */
int lcc_event_add_event_produced(struct lcc_event_context* ctx,
                                   uint64_t event_id);

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


#ifdef __cplusplus
} /* extern C */
#endif

#endif /* LIBLCC_EVENT_H */
