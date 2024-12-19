/* SPDX-License-Identifier: GPL-2.0 */
#include <stdlib.h>
#include <string.h>

#include "lcc-simple.h"
#include "lcc-common-internal.h"

static int lcc_handle_producer_query(struct lcc_context* ctx, struct lcc_can_frame* frame){
    if(!ctx->write_function ||
            !ctx->event_context){
        return LCC_OK;
    }

    if(frame->can_len != 8){
        return LCC_ERROR_EVENT_ID_INVALID;
    }

    uint64_t event_id = lcc_get_eventid_from_data(frame);
    enum lcc_producer_state state = LCC_PRODUCER_UNKNOWN;

    if(event_list_has_event(&ctx->event_context->events_produced, event_id)){
        struct lcc_can_frame frame;

        if(ctx->event_context->producer_state_fn){
            state = ctx->event_context->producer_state_fn(ctx, event_id);
        }

        memset(&frame, 0, sizeof(frame));
        switch(state){
        case LCC_PRODUCER_VALID:
            lcc_set_lcb_variable_field(&frame, ctx, LCC_MTI_PRODUCER_IDENTIFIED_VALID | LCC_MTI_EVENT_NUM_PRESENT);
            break;
        case LCC_PRODUCER_INVALID:
            lcc_set_lcb_variable_field(&frame, ctx, LCC_MTI_PRODUCER_IDENTIFIED_INVALID | LCC_MTI_EVENT_NUM_PRESENT);
            break;
        case LCC_PRODUCER_UNKNOWN:
            lcc_set_lcb_variable_field(&frame, ctx, LCC_MTI_PRODUCER_IDENTIFIED_UNKNOWN | LCC_MTI_EVENT_NUM_PRESENT);
            break;
        }

        lcc_set_lcb_can_frame_type(&frame, 1);
        lcc_set_eventid_in_data(&frame, event_id);
        return ctx->write_function(ctx, &frame);
    }

    return LCC_OK;
}

static int lcc_handle_consumer_query(struct lcc_context* ctx, struct lcc_can_frame* frame){
    if(!ctx->write_function ||
            !ctx->event_context){
        return LCC_OK;
    }

    if(frame->can_len != 8){
        return LCC_ERROR_EVENT_ID_INVALID;
    }

    uint64_t event_id = lcc_get_eventid_from_data(frame);

    if(event_list_has_event(&ctx->event_context->events_consumed, event_id)){
        /*
         * Currently valid – the internal state of the consumer & associated devices is known to be the same as if this was the last event consumed
         * Currently invalid – the internal state of the consumer & associated devices is known to not be the same as if this was the last event consumed
         * Currently unknown – the consumer cannot determine whether either of the previous conditions is true
         */

        // We consume this event..  not sure how to handle this correctly, but let's just go with 'unknown'
        struct lcc_can_frame frame;
        memset(&frame, 0, sizeof(frame));

        if(ctx->event_context->consumer_state_fn){
            enum lcc_consumer_state state = ctx->event_context->consumer_state_fn(ctx, event_id);
            switch(state){
            case LCC_CONSUMER_VALID:
                lcc_set_lcb_variable_field(&frame, ctx, LCC_MTI_CONSUMER_IDENTIFIED_VALID| LCC_MTI_EVENT_NUM_PRESENT);
                break;
            case LCC_CONSUMER_INVALID:
                lcc_set_lcb_variable_field(&frame, ctx, LCC_MTI_CONSUMER_IDENTIFIED_INVALID | LCC_MTI_EVENT_NUM_PRESENT);
                break;
            case LCC_CONSUMER_UNKNOWN:
                lcc_set_lcb_variable_field(&frame, ctx, LCC_MTI_CONSUMER_IDENTIFIED_UNKNOWN | LCC_MTI_EVENT_NUM_PRESENT);
                break;
            }
        }else{
            lcc_set_lcb_variable_field(&frame, ctx, LCC_MTI_CONSUMER_IDENTIFIED_UNKNOWN | LCC_MTI_EVENT_NUM_PRESENT);
        }

        lcc_set_lcb_can_frame_type(&frame, 1);
        lcc_set_eventid_in_data(&frame, event_id);
        return ctx->write_function(ctx, &frame);
    }

    return LCC_OK;
}

static int lcc_handle_producer_consumer(struct lcc_context* ctx, struct lcc_can_frame* frame){
    if(!ctx->event_context->incoming_event){
        // There's no method to call, let's bail immediately
        return LCC_OK;
    }

    if(frame->can_len != 8){
        return LCC_ERROR_EVENT_ID_INVALID;
    }

    uint64_t event_id = lcc_get_eventid_from_data(frame);

    if(ctx->listen_all_events){
        ctx->event_context->incoming_event(ctx, event_id);
    }
    if(event_list_has_event(&ctx->event_context->events_consumed, event_id)){
        ctx->event_context->incoming_event(ctx, event_id);
    }

    return LCC_OK;
}

int lcc_handle_simple_protocol(struct lcc_context* ctx, struct lcc_can_frame* frame){
    if(ctx == NULL || frame == NULL){
        return LCC_ERROR_INVALID_ARG;
    }

    uint16_t mti = (frame->can_id & LCC_VARIABLE_FIELD_MASK) >> 12;

    if((mti & LCC_MTI_SIMPLE) == 0){
        // This is not a simple frame
        return LCC_ERROR_GENERIC;
    }

    if(mti == LCC_MTI_BASIC_VERIFY_NODE_ID_NUM_GLOBAL){
        // Check to see if this contains a node ID.  If it does and it doesn't match,
        // don't send anything back
        if(frame->can_len == 6){
            uint64_t addressed_data = lcc_get_node_id_from_data(frame);
            if(addressed_data != ctx->unique_id){
                return LCC_OK;
            }
        }

        // Respond with frame ID
        struct lcc_can_frame frame;
        memset(&frame, 0, sizeof(frame));
        lcc_set_lcb_variable_field(&frame, ctx, LCC_MTI_BASIC_VERIFIED_NODE_ID_NUM | LCC_MTI_SIMPLE);
        lcc_set_lcb_can_frame_type(&frame, 1);
        lcc_set_nodeid_in_data(&frame, ctx->unique_id);
        ctx->write_function(ctx, &frame);
        return LCC_OK;
    }else if(mti == LCC_MTI_EVENT_IDENTIFY_CONSUMER){
        return lcc_handle_consumer_query(ctx, frame);
    }else if(mti == LCC_MTI_EVENT_IDENTIFY_PRODUCER){
        return lcc_handle_producer_query(ctx, frame);
    }else if(mti == LCC_MTI_EVENT_IDENTIFY_EVENTS_GLOBAL){
        return lcc_send_events_produced(ctx);
    }else if(mti == LCC_MTI_PRODUCER_CONSUMER_EVENT_REPORT){
        return lcc_handle_producer_consumer(ctx, frame);
    }

    return LCC_OK;
}
