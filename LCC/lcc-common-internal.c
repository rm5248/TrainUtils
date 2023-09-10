/* SPDX-License-Identifier: GPL-2.0 */
#include <stdlib.h>
#include <string.h>
#include "lcc-common-internal.h"

void lcc_set_lcb_variable_field(struct lcc_can_frame* frame, struct lcc_context* ctx, int variable_field){
    uint32_t var_field_u32 = variable_field;
    frame->can_id = 0;
    frame->can_id |= (0x01l << 28); /* reserved, set as 1 */
    frame->can_id |= (0x01l << 27); /* openLCB frame type */
    frame->can_id |= (var_field_u32 << 12);
    frame->can_id |= (ctx->node_alias & 0xFFF);
}

void lcc_set_lcb_can_frame_type(struct lcc_can_frame* frame, int type){
    uint32_t type_field_u32 = type;
    frame->can_id |= (type_field_u32 << 24);
}

void lcc_set_nodeid_in_data(struct lcc_can_frame* frame, uint64_t node_id){
    frame->can_len = 6;
    frame->data[0] = (node_id & 0xFF0000000000l) >> 40;
    frame->data[1] = (node_id & 0x00FF00000000l) >> 32;
    frame->data[2] = (node_id & 0x0000FF000000l) >> 24;
    frame->data[3] = (node_id & 0x000000FF0000l) >> 16;
    frame->data[4] = (node_id & 0x00000000FF00l) >> 8;
    frame->data[5] = (node_id & 0x0000000000FFl) >> 0;
}

uint64_t lcc_get_node_id_from_data(struct lcc_can_frame* frame){
    uint64_t ret = 0;

    ret |= ((uint64_t)frame->data[0]) << 40;
    ret |= ((uint64_t)frame->data[1]) << 32;
    ret |= ((uint64_t)frame->data[2]) << 24;
    ret |= ((uint64_t)frame->data[3]) << 16;
    ret |= ((uint64_t)frame->data[4]) << 8;
    ret |= ((uint64_t)frame->data[5]) << 0;

    return ret;
}

void lcc_set_flags_and_dest_alias(struct lcc_can_frame* frame, int flag_frame, int alias){
    frame->data[0] = (flag_frame << 4) | ((alias & 0xF00) >> 8);
    frame->data[1] = (alias & 0xFF);
}

void lcc_set_eventid_in_data(struct lcc_can_frame* frame, uint64_t event_id){
    frame->can_len = 8;
    frame->data[0] = (event_id & 0xFF00000000000000l) >> 56;
    frame->data[1] = (event_id & 0x00FF000000000000l) >> 48;
    frame->data[2] = (event_id & 0x0000FF0000000000l) >> 40;
    frame->data[3] = (event_id & 0x000000FF00000000l) >> 32;
    frame->data[4] = (event_id & 0x00000000FF000000l) >> 24;
    frame->data[5] = (event_id & 0x0000000000FF0000l) >> 16;
    frame->data[6] = (event_id & 0x000000000000FF00l) >> 8;
    frame->data[7] = (event_id & 0x00000000000000FFl) >> 0;
}

uint64_t lcc_get_eventid_from_data(struct lcc_can_frame* frame){
    uint64_t ret = 0;

    ret |= ((uint64_t)frame->data[0]) << 56;
    ret |= ((uint64_t)frame->data[1]) << 48;
    ret |= ((uint64_t)frame->data[2]) << 40;
    ret |= ((uint64_t)frame->data[3]) << 32;
    ret |= ((uint64_t)frame->data[4]) << 24;
    ret |= ((uint64_t)frame->data[5]) << 16;
    ret |= ((uint64_t)frame->data[6]) << 8;
    ret |= ((uint64_t)frame->data[7]) << 0;

    return ret;
}

static int event_list_compare(const void * arg1, const void * arg2){
    uint64_t id1 = *(const uint64_t*)arg1;
    uint64_t id2 = *(const uint64_t*)arg2;

    if (id1 < id2) return -1;
    if (id1 > id2) return 1;
    return 0;
}

void event_list_add_event(struct event_list* list, uint64_t event_id){
#if defined(LIBLCC_EVENT_LIST_STATIC_SIZE)
    if(list->len > list->size){
        return;
    }
    list->event_array[list->len] = event_id;
    list->len++;
#else
    if(list->event_array == NULL){
        list->event_array = malloc(sizeof(int64_t) * 10);
        list->size = 10;
    }

    list->event_array[list->len] = event_id;
    list->len++;
    if(list->len == list->size){
        // Make a new array
        int newSize = list->size * 2;
        uint64_t* new_array = malloc(sizeof(int64_t) * newSize);
        memcpy(new_array, list->event_array, sizeof(uint64_t) * list->size);
        free(list->event_array);
        list->event_array = new_array;
        list->size = newSize;
    }
#endif

    // Sort all of the event IDs to make the search(when they come in) easier
    qsort(list->event_array, list->len, sizeof(int64_t), event_list_compare);
}

int event_list_has_event(struct event_list* list, uint64_t event_id){
    if(list->event_array == NULL){
        return 0;
    }

    void* found_event = bsearch(&event_id, list->event_array, list->len, sizeof(uint64_t), event_list_compare);
    if(found_event == NULL){
        return 0;
    }

    return 1;
}

int lcc_send_events_produced(struct lcc_context* ctx){
    struct lcc_can_frame frame;
    enum lcc_producer_state state = LCC_PRODUCER_UNKNOWN;
    uint64_t event_id;

    if(!ctx->write_function ||
            !ctx->event_context){
        return LCC_OK;
    }

    for(int x = 0; x < ctx->event_context->events_produced.len; x++){
        state = LCC_PRODUCER_UNKNOWN;
        event_id = ctx->event_context->events_produced.event_array[x];
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
        int write_ret = ctx->write_function(ctx, &frame);
        if(write_ret != LCC_OK){
            return write_ret;
        }
    }

    return LCC_OK;
}

uint32_t lcc_uint32_from_data(void* data){
    uint32_t retval = 0;
    uint8_t* u8_data = data;

    retval = (u8_data[0] << 24l) |
            (u8_data[1] << 16l) |
            (u8_data[2] << 8l) |
            (u8_data[3]);
    return retval;
}

void lcc_uint32_to_data(void* data, uint32_t value){
    uint8_t* u8_data = data;
    u8_data[0] = ((value & 0xFF000000) >> 24l) & 0xFF;
    u8_data[1] = ((value & 0x00FF0000) >> 16l) & 0xFF;
    u8_data[2] = ((value & 0x0000FF00) >> 8l) & 0xFF;
    u8_data[3] = ((value & 0x000000FF) >> 0l) & 0xFF;
}
