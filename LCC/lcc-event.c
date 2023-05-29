/* SPDX-License-Identifier: GPL-2.0 */
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "lcc-event.h"
#include "lcc-common-internal.h"

struct lcc_event_context* lcc_event_new(struct lcc_context* parent){
#ifdef ARDUINO
    static struct lcc_event_context event_ctx;
    memset(&event_ctx, 0, sizeof(event_ctx));
    event_ctx.parent = parent;
    parent->event_context = &event_ctx;

    event_ctx.events_consumed.size = sizeof(event_ctx.events_consumed.event_array) / sizeof(event_ctx.events_consumed.event_array[0]);
    event_ctx.events_produced.size = sizeof(event_ctx.events_produced.event_array) / sizeof(event_ctx.events_produced.event_array[0]);

    return &event_ctx;
#else
    struct lcc_event_context* event_ctx = malloc(sizeof(struct lcc_event_context));

    memset(event_ctx, 0, sizeof(struct lcc_event_context));
    event_ctx->parent = parent;
    parent->event_context = event_ctx;

    return event_ctx;
#endif
}

int lcc_event_set_incoming_event_function(struct lcc_event_context* ctx,
                                            lcc_incoming_event_fn fn){
    if(!ctx){
        return LCC_ERROR_INVALID_ARG;
    }

    ctx->incoming_event = fn;
    return LCC_OK;
}

int lcc_event_add_event_consumed(struct lcc_event_context* ctx,
                                   uint64_t event_id){
    if(!ctx){
        return LCC_ERROR_INVALID_ARG;
    }

    event_list_add_event(&ctx->events_consumed, event_id);

    return LCC_OK;
}

int lcc_event_add_event_produced(struct lcc_event_context* ctx,
                                   uint64_t event_id){
    if(!ctx){
        return LCC_ERROR_INVALID_ARG;
    }

    event_list_add_event(&ctx->events_produced, event_id);

    return LCC_OK;
}

int lcc_event_set_listen_all_events(struct lcc_event_context* ctx,
                                      int listen_all){
    if(!ctx){
        return LCC_ERROR_INVALID_ARG;
    }

    if(listen_all){
        ctx->parent->listen_all_events = 1;
    }else{
        ctx->parent->listen_all_events = 0;
    }

    return LCC_OK;
}

int lcc_event_add_event_produced_query_fn(struct lcc_event_context* ctx,
                                            lcc_query_producer_state_fn producer_state){
    if(!ctx){
        return LCC_ERROR_INVALID_ARG;
    }

    ctx->producer_state_fn = producer_state;

    return LCC_OK;
}

int lcc_event_produce_event(struct lcc_event_context* ctx,
                              uint64_t event_id){
    if(!ctx){
        return LCC_ERROR_INVALID_ARG;
    }

//    if(!ctx->write_function){
//        return LCC_OK;
//    }

    struct lcc_can_frame frame;
    memset(&frame, 0, sizeof(frame));

    lcc_set_lcb_variable_field(&frame, ctx->parent, LCC_MTI_PRODUCER_CONSUMER_EVENT_REPORT | LCC_MTI_SIMPLE | LCC_MTI_EVENT_NUM_PRESENT);
    lcc_set_lcb_can_frame_type(&frame, 1);
    lcc_set_eventid_in_data(&frame, event_id);
    ctx->parent->write_function(ctx->parent, &frame);

    return LCC_OK;
}
