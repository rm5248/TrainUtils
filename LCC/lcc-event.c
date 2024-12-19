/* SPDX-License-Identifier: GPL-2.0 */
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "lcc-event.h"
#include "lcc-common-internal.h"

struct lcc_event_context* lcc_event_new(struct lcc_context* parent){
#ifdef LIBLCC_ENABLE_STATIC_CONTEXT
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

    event_list_add_event(&ctx->events_consumed, event_id, !ctx->in_add_consumed_event_transaction);

    return LCC_OK;
}

int lcc_event_remove_event_consumed(struct lcc_event_context* ctx,
                                    uint64_t event_id){
    if(!ctx){
        return LCC_ERROR_INVALID_ARG;
    }

    event_list_remove_event(&ctx->events_consumed, event_id);

    return LCC_OK;
}

int lcc_event_clear_events(struct lcc_event_context* ctx, int event_flags){
    if(!ctx){
        return LCC_ERROR_INVALID_ARG;
    }

    if(event_flags & LCC_EVENT_CONTEXT_CLEAR_EVENTS_PRODUCED){
        event_list_clear(&ctx->events_produced);
    }

    if(event_flags & LCC_EVENT_CONTEXT_CLEAR_EVENTS_CONSUMED){
        event_list_clear(&ctx->events_consumed);
    }

    return LCC_OK;
}

int lcc_event_add_event_consumed_query_fn(struct lcc_event_context* ctx,
                                          lcc_query_consumer_state_fn consumer_state){
    if(!ctx){
        return LCC_ERROR_INVALID_ARG;
    }

    ctx->consumer_state_fn = consumer_state;

    return LCC_OK;
}

int lcc_event_add_event_produced(struct lcc_event_context* ctx,
                                   uint64_t event_id){
    if(!ctx){
        return LCC_ERROR_INVALID_ARG;
    }

    event_list_add_event(&ctx->events_produced, event_id, !ctx->in_add_produced_event_transaction);

    return LCC_OK;
}

int lcc_event_add_event_produced_transaction_start(struct lcc_event_context* ctx){
    if(!ctx){
        return LCC_ERROR_INVALID_ARG;
    }

    ctx->in_add_produced_event_transaction = 1;

    return LCC_OK;
}
int lcc_event_add_event_produced_transaction_end(struct lcc_event_context* ctx){
    if(!ctx){
        return LCC_ERROR_INVALID_ARG;
    }

    ctx->in_add_produced_event_transaction = 0;

    event_list_sort(&ctx->events_produced);

    return LCC_OK;
}

int lcc_event_add_event_consumed_transaction_start(struct lcc_event_context* ctx){
    if(!ctx){
        return LCC_ERROR_INVALID_ARG;
    }

    ctx->in_add_consumed_event_transaction = 1;

    return LCC_OK;
}

int lcc_event_add_event_consumed_transaction_end(struct lcc_event_context* ctx){
    if(!ctx){
        return LCC_ERROR_INVALID_ARG;
    }

    ctx->in_add_consumed_event_transaction = 0;

    event_list_sort(&ctx->events_consumed);

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
    return ctx->parent->write_function(ctx->parent, &frame);
}

int lcc_event_id_is_accessory_address(uint64_t event_id){
    const uint64_t event_mask = 0x0101020000FF0000;

    if((event_id & event_mask) == event_mask){
        return 1;
    }

    return 0;
}

int lcc_event_id_to_accessory_decoder(uint64_t event_id, struct lcc_accessory_address* address){
    if(!lcc_event_id_is_accessory_address(event_id)){
        return LCC_ERROR_EVENT_NOT_ACCESSORY_DECODER;
    }

    if(address == NULL){
        return LCC_ERROR_INVALID_ARG;
    }

    uint16_t lower_bits = event_id & 0xFFFF;
    int active_bit = lower_bits & (0x01 << 3);
    uint8_t direction = (event_id & 0xFF0000ll) >> 16;

    // Remove bit 3 from our address
    uint16_t upper_bits = 0xFF0;
    upper_bits = upper_bits >> 1;

    address->dcc_accessory_address = upper_bits | (lower_bits & 0x7);
    if(direction == 0xFF && active_bit){
        address->active = 1;
    }else{
        address->active = 0;
    }

    return LCC_OK;
}

int lcc_event_id_to_accessory_decoder_2040(uint64_t event_id, struct lcc_accessory_address* address){
    if(!lcc_event_id_is_accessory_address(event_id)){
        return LCC_ERROR_EVENT_NOT_ACCESSORY_DECODER;
    }

    if(address == NULL){
        return LCC_ERROR_INVALID_ARG;
    }

    // https://docs.tcsdcc.com/wiki/DCC_Turnout_Creation_in_JMRI_With_TCS_Command_Stations#LCC_Event_ID_Reference_Table
    uint16_t addr = event_id & 0xFFF;
    addr = addr - 0x8;
    addr = addr >> 1;

    int active = event_id & 0x01;

    uint8_t direction = (event_id & 0xFF0000ll) >> 16;

    // DCC addresses are 1-indexed
    address->dcc_accessory_address = addr + 1;
    if(direction == 0xFF && active){
        address->active = 1;
    }else{
        address->active = 0;
    }

    return LCC_OK;
}

int lcc_accessory_decoder_to_event_id_2040(struct lcc_accessory_address* address, uint64_t* event_id){
	if(address == NULL || event_id == NULL){
		return LCC_ERROR_INVALID_ARG;
	}

    if(address->dcc_accessory_address > 2048 || address->dcc_accessory_address == 0){
		return LCC_ERROR_INVALID_ARG;
	}

    *event_id = 0x0101020000FF0000llu;

    uint16_t addr = 0;
    if(address->dcc_accessory_address < 2044){
        addr = address->dcc_accessory_address << 1;
        addr = addr + 0x6;
    }else{
        addr = ((address->dcc_accessory_address - 1) - 2044) << 1;
    }
    if(address->active){
        addr++;
    }

    *event_id |= addr;

    return LCC_OK;
}
