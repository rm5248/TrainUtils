/* SPDX-License-Identifier: GPL-2.0 */
#include <stdlib.h>
#include <string.h>

#include "lcc.h"
#include "lcc-common-internal.h"
#include "lcc-common.h"

static void lcc_set_lcb_variable_field(struct lcc_can_frame* frame, struct lcc_context* ctx, int variable_field){
    frame->can_id = 0;
    frame->can_id |= (0x01 << 28); /* reserved, set as 1 */
    frame->can_id |= (0x01 << 27); /* openLCB frame type */
    frame->can_id |= (variable_field << 12);
    frame->can_id |= (ctx->node_alias & 0xFFF);
}

static void lcc_set_lcb_can_frame_type(struct lcc_can_frame* frame, int type){
    frame->can_id |= (type << 24);
}

static void lcc_set_nodeid_in_data(struct lcc_can_frame* frame, uint64_t node_id){
    frame->can_len = 6;
    frame->data[0] = (node_id & 0xFF0000000000l) >> 40;
    frame->data[1] = (node_id & 0x00FF00000000l) >> 32;
    frame->data[2] = (node_id & 0x0000FF000000l) >> 24;
    frame->data[3] = (node_id & 0x000000FF0000l) >> 16;
    frame->data[4] = (node_id & 0x00000000FF00l) >> 8;
    frame->data[5] = (node_id & 0x0000000000FFl) >> 0;
}

struct lcc_context* lcc_context_new(){
    struct lcc_context* newctx = malloc(sizeof(struct lcc_context));

    if( !newctx ){
        return NULL;
    }

    memset(newctx, 0, sizeof(struct lcc_context));

    newctx->state = LCC_STATE_INHIBITED;

    return newctx;
}

void lcc_context_free(struct lcc_context* ctx){
    if( !ctx ){
        return;
    }

    free(ctx);
}

int lcc_context_incoming_frame(struct lcc_context* ctx, struct lcc_can_frame* frame){
    if( !frame || !ctx ){
        return LCC_ERROR_INVALID_ARG;
    }

    uint16_t node_alias = frame->can_id & 0xFFF;
    if( ctx->node_alias_state == LCC_NODE_ALIAS_SENT_CID &&
            node_alias == ctx->node_alias ){
        ctx->node_alias_state = LCC_NODE_ALIAS_FAIL;
    }

    if(ctx->state != LCC_STATE_PERMITTED){
        return LCC_OK;
    }

    // Decode the CAN frame and maybe do something useful with it.
    if((frame->can_id & LCC_FRAME_TYPE_MASK) == 0){
        // TODO need to be smart about non LCC frames
        return LCC_OK;
    }

    uint16_t mti = (frame->can_id & LCC_VARIABLE_FIELD_MASK) >> 12;
    uint8_t frame_type = (frame->can_id & LCC_CAN_FRAME_TYPE_MASK) >> 24;
    uint16_t source_alias = (frame->can_id & LCC_NID_ALIAS_MASK);

    if(frame_type != 1){
        return LCC_OK;
    }

    if(mti == LCC_MTI_BASIC_VERIFY_NODE_ID_NUM_GLOBAL){
        // Respond with frame ID
        struct lcc_can_frame frame;
        memset(&frame, 0, sizeof(frame));
        lcc_set_lcb_variable_field(&frame, ctx, LCC_MTI_BASIC_VERIFIED_NODE_ID_NUM);
        lcc_set_lcb_can_frame_type(&frame, 1);
        lcc_set_nodeid_in_data(&frame, ctx->unique_id);
        ctx->write_function(ctx, &frame);
    }

    return LCC_OK;
}

int lcc_context_set_write_function(struct lcc_context* ctx, lcc_write_fn write_fn){
    if( !write_fn || !ctx ){
        return LCC_ERROR_INVALID_ARG;
    }

    ctx->write_function = write_fn;

    return LCC_OK;
}

int lcc_context_set_unique_identifer(struct lcc_context* ctx, uint64_t id){
    if( !ctx ){
        return LCC_ERROR_INVALID_ARG;
    }

    ctx->unique_id = id & 0xFFFFFFFFFFFFl;

    return LCC_OK;
}

uint64_t lcc_context_unique_id(struct lcc_context* ctx){
    if( !ctx ){
        return LCC_ERROR_INVALID_ARG;
    }

    return ctx->unique_id;
}

int lcc_context_generate_alias(struct lcc_context* ctx){
    if( !ctx ){
        return LCC_ERROR_INVALID_ARG;
    }

    if( ctx->unique_id == 0 ){
        // We need to have a valid unique ID
        return LCC_ERROR_UNIQUE_ID_NOT_SET;
    }

    if( ctx->node_alias_state == LCC_NODE_ALIAS_GOOD ){
        // Alias has already been set(invalid state)
        return LCC_ERROR_ALIAS_SET;
    }

    if( ctx->node_alias == 0 ){
        // Let's generate a starting alias number.
        ctx->node_alias = ctx->unique_id & 0xFFF;
        ctx->node_alias ^= 0xA5C;
    }else{
        ctx->node_alias++;
    }

    if( ctx->node_alias == 0xFFF ){
        // Node alias is only 12 bits.
        ctx->node_alias = 1;
    }

    if( ctx->node_alias == 0 ){
        ctx->node_alias++;
    }

    // Send our four CID frames
    struct lcc_can_frame frame;
    frame.can_len = 0;
    frame.res0 = 0;
    frame.res1 = 0;
    frame.res2 = 0;

    lcc_set_lcb_variable_field(&frame, ctx, ((ctx->unique_id & 0xFFF000000000l) >> 36) | 0x7000);
    // this is a CAN control frame, clear bit 27
    frame.can_id &= (~(0x01 << 27));
    ctx->write_function(ctx, &frame);

    lcc_set_lcb_variable_field(&frame, ctx, ((ctx->unique_id & 0x000FFF000000l) >> 24) | 0x6000);
    frame.can_id &= (~(0x01 << 27));
    ctx->write_function(ctx, &frame);

    lcc_set_lcb_variable_field(&frame, ctx, ((ctx->unique_id & 0x000000FFF000l) >> 12) | 0x5000);
    frame.can_id &= (~(0x01 << 27));
    ctx->write_function(ctx, &frame);

    lcc_set_lcb_variable_field(&frame, ctx, ((ctx->unique_id & 0x000000000FFFl) >> 0) | 0x4000);
    frame.can_id &= (~(0x01 << 27));
    ctx->write_function(ctx, &frame);

    ctx->node_alias_state = LCC_NODE_ALIAS_SENT_CID;

    return LCC_OK;
}

int lcc_context_claim_alias(struct lcc_context* ctx){
    if( !ctx ){
        return LCC_ERROR_INVALID_ARG;
    }

    if( ctx->node_alias_state != LCC_NODE_ALIAS_SENT_CID ){
        return LCC_ERROR_ALIAS_FAILURE;
    }

    ctx->node_alias_state = LCC_NODE_ALIAS_GOOD;
    ctx->state = LCC_STATE_PERMITTED;

    // Now send the RID frame and AMD frame
    struct lcc_can_frame frame;
    frame.can_len = 0;
    frame.res0 = 0;
    frame.res1 = 0;
    frame.res2 = 0;

    // RID frame
    lcc_set_lcb_variable_field(&frame, ctx, 0x700);
    frame.can_id &= (~(0x01 << 27));
    ctx->write_function(ctx, &frame);

    // AMD frame
    lcc_set_lcb_variable_field(&frame, ctx, 0x701);
    frame.can_id &= (~(0x01 << 27));
    lcc_set_nodeid_in_data(&frame, ctx->unique_id);
    ctx->write_function(ctx, &frame);

    return LCC_OK;
}

void lcc_context_set_userdata(struct lcc_context* ctx, void* user_data){
    if(!ctx) return;
    ctx->user_data = user_data;
}

void* lcc_context_user_data(struct lcc_context* ctx){
    if(!ctx) return NULL;
    return ctx->user_data;
}

int lcc_context_alias(struct lcc_context* ctx){
    if(ctx == NULL) return 0;
    return ctx->node_alias;
}
