/* SPDX-License-Identifier: GPL-2.0 */
#include <stdlib.h>
#include <string.h>
#include "lcc-node-info.h"
#include "lcc-network-internal.h"

struct lcc_node_info* lcc_node_info_new(struct lcc_context* parent){
#ifdef ARDUINO
    static struct lcc_node_info inf;
    memset(&inf, 0, sizeof(struct lcc_node_info));
    return &inf;
#else
    struct lcc_node_info* inf = malloc(sizeof(struct lcc_node_info));

    memset(inf, 0, sizeof(struct lcc_node_info));
    inf->parent_ctx = parent;

    return inf;
#endif
}

void lcc_node_info_free(struct lcc_node_info* inf){
#ifndef ARDUINO
    free(inf);
#endif
}

uint64_t lcc_node_info_get_id(struct lcc_node_info* inf){
    if(!inf) return 0;

    return inf->node_id;
}

int lcc_node_info_get_alias(struct lcc_node_info* inf){
    if(!inf) return 0;
    return inf->node_alias;
}

int lcc_node_info_get_events_produced(struct lcc_node_info* inf, uint64_t** produced_list, int* produced_len){
    if(!inf || !produced_list || !produced_len) return LCC_ERROR_INVALID_ARG;

    *produced_list = inf->produced_events.event_array;
    *produced_len = inf->produced_events.len;

    return LCC_OK;
}

int lcc_node_info_get_events_consumed(struct lcc_node_info* inf, uint64_t** consumed_list, int* consumed_len){
    if(!inf || !consumed_list || !consumed_len) return LCC_ERROR_INVALID_ARG;

    *consumed_list = inf->consumed_events.event_array;
    *consumed_len = inf->consumed_events.len;

    return LCC_OK;
}

struct lcc_simple_node_info* lcc_node_info_get_simple(struct lcc_node_info* inf){
    if(!inf) return NULL;
    return &inf->simple_info;
}

int lcc_node_info_get_protocols_supported(struct lcc_node_info* inf, enum lcc_protocols** protocols_list, int* protocols_len){
    if(!inf || !protocols_list || !protocols_len) return LCC_ERROR_INVALID_ARG;

    int protocols_list_len;
    for(protocols_list_len = 0;
        protocols_list_len < (sizeof(inf->protocol_list) / sizeof(inf->protocol_list[0]));
        protocols_list_len++){
        if(inf->protocol_list[protocols_list_len] == LCC_PROTOCOL_INVALID){
            break;
        }
    }
    *protocols_len = protocols_list_len;
    *protocols_list = inf->protocol_list;

    return LCC_OK;
}

int lcc_node_refresh_simple_info(struct lcc_node_info* inf){
    if(!inf) return LCC_ERROR_INVALID_ARG;
    struct lcc_can_frame frame;
    memset(&frame, 0, sizeof(frame));

    lcc_set_lcb_variable_field(&frame, inf->parent_ctx, LCC_MTI_SIMPLE_NODE_INFORMATION_REQUEST | LCC_MTI_ADDRESSED);
    lcc_set_lcb_can_frame_type(&frame, 1);
    frame.can_len = 2;
    frame.data[0] = (inf->node_alias & 0xFF00) >> 8;
    frame.data[1] = (inf->node_alias & 0x00FF);

    return inf->parent_ctx->write_function(inf->parent_ctx, &frame);
}

int lcc_node_refresh_events_produced(struct lcc_node_info* inf){
    if(!inf) return LCC_ERROR_INVALID_ARG;
    struct lcc_can_frame frame;
    memset(&frame, 0, sizeof(frame));

    lcc_set_lcb_variable_field(&frame, inf->parent_ctx, LCC_MTI_EVENT_IDENTIFY_PRODUCER | LCC_MTI_ADDRESSED);
    lcc_set_lcb_can_frame_type(&frame, 1);
    frame.can_len = 2;
    frame.data[0] = (inf->node_alias & 0xFF00) >> 8;
    frame.data[1] = (inf->node_alias & 0x00FF);

    return inf->parent_ctx->write_function(inf->parent_ctx, &frame);
}

int lcc_node_refresh_events_consumed(struct lcc_node_info* inf){
    if(!inf) return LCC_ERROR_INVALID_ARG;
    struct lcc_can_frame frame;
    memset(&frame, 0, sizeof(frame));

    lcc_set_lcb_variable_field(&frame, inf->parent_ctx, LCC_MTI_EVENT_IDENTIFY_CONSUMER | LCC_MTI_ADDRESSED);
    lcc_set_lcb_can_frame_type(&frame, 1);
    frame.can_len = 2;
    frame.data[0] = (inf->node_alias & 0xFF00) >> 8;
    frame.data[1] = (inf->node_alias & 0x00FF);

    return inf->parent_ctx->write_function(inf->parent_ctx, &frame);
}

int lcc_node_refresh_protocol_support(struct lcc_node_info* inf){
    if(!inf) return LCC_ERROR_INVALID_ARG;
    struct lcc_can_frame frame;
    memset(&frame, 0, sizeof(frame));

    lcc_set_lcb_variable_field(&frame, inf->parent_ctx, LCC_MTI_PROTOCOL_SUPPORT_INQUIRE | LCC_MTI_ADDRESSED);
    lcc_set_lcb_can_frame_type(&frame, 1);
    frame.can_len = 2;
    frame.data[0] = (inf->node_alias & 0xFF00) >> 8;
    frame.data[1] = (inf->node_alias & 0x00FF);

    return inf->parent_ctx->write_function(inf->parent_ctx, &frame);
}
