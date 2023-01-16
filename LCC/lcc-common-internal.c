/* SPDX-License-Identifier: GPL-2.0 */
#include "lcc-common-internal.h"

void lcc_set_lcb_variable_field(struct lcc_can_frame* frame, struct lcc_context* ctx, int variable_field){
    frame->can_id = 0;
    frame->can_id |= (0x01 << 28); /* reserved, set as 1 */
    frame->can_id |= (0x01 << 27); /* openLCB frame type */
    frame->can_id |= (variable_field << 12);
    frame->can_id |= (ctx->node_alias & 0xFFF);
}

void lcc_set_lcb_can_frame_type(struct lcc_can_frame* frame, int type){
    frame->can_id |= (type << 24);
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
