/* SPDX-License-Identifier: GPL-2.0 */
#include "lcc-addressed.h"
#include "lcc-common-internal.h"

int lcc_handle_addressed(struct lcc_context* ctx, struct lcc_can_frame* frame){
    uint16_t alias = (frame->data[0] << 8) | frame->data[1];
    if(alias != ctx->node_alias){
        return LCC_OK;
    }

    uint16_t mti = (frame->can_id & LCC_VARIABLE_FIELD_MASK) >> 12;
    uint16_t sender = (frame->can_id & LCC_NID_ALIAS_MASK);
    struct lcc_can_frame ret_frame;
    memset(&ret_frame, 0, sizeof(ret_frame));

    if(mti == LCC_MTI_BASIC_VERIFY_NODE_ID_NUM_ADDRESSED){
        // Respond with Node ID
        lcc_set_lcb_variable_field(&ret_frame, ctx, LCC_MTI_BASIC_VERIFIED_NODE_ID_NUM | LCC_MTI_SIMPLE);
        lcc_set_lcb_can_frame_type(&ret_frame, 1);
        lcc_set_nodeid_in_data(&ret_frame, ctx->unique_id);
        ctx->write_function(ctx, &ret_frame);
        return LCC_OK;
    }

    if(mti == LCC_MTI_PROTOCOL_SUPPORT_INQUIRE){
        // Respond with protocol support reply
        ret_frame.can_len = 3;
        ret_frame.data[0] = (sender & 0xFF00) >> 8;
        ret_frame.data[1] = (sender & 0x00FF);
        ret_frame.data[2] = 0x80; // For now just say simple protocol
        lcc_set_lcb_variable_field(&ret_frame, ctx, LCC_MTI_PROTOCOL_SUPPORT_REPLY | LCC_MTI_ADDRESSED);
        lcc_set_lcb_can_frame_type(&ret_frame, 1);
        ctx->write_function(ctx, &ret_frame);
        return LCC_OK;
    }

    return LCC_OK;
}
