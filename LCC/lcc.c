/* SPDX-License-Identifier: GPL-2.0 */
#include <stdlib.h>
#include <string.h>

#include "lcc.h"

#define LCC_FRAME_TYPE_MASK     0x08000000
#define LCC_VARIABLE_FIELD_MASK 0x07FFF000
#define LCC_NID_ALIAS_MASK      0x00000FFF

#define LCC_STATE_INHIBITED 1
#define LCC_STATE_PERMITTED 0

#define LCC_NODE_ALIAS_NOT_SET 0
#define LCC_NODE_ALIAS_SENT_CID 1
#define LCC_NODE_ALIAS_FAIL 2
#define LCC_NODE_ALIAS_GOOD 3

struct lcc_context{
    uint64_t unique_id;
    union{
        int16_t flags;
        int16_t reserved : 13, node_alias_state : 2, state : 1;
    };
    int16_t node_alias;
    lcc_write_fn write_function;
};

static void lcc_set_lcb_variable_field(struct lcc_can_frame* frame, struct lcc_context* ctx, int variable_field){
    frame->can_id = 0;
    frame->can_id |= (0x01 << 28); /* reserved, set as 1 */
    frame->can_id |= (0x01 << 27); /* openLCB frame type */
    frame->can_id |= (variable_field << 12);
    frame->can_id |= (ctx->node_alias & 0xFFF);
}

struct lcc_context* lcc_context_new(){
    struct lcc_context* newctx = malloc(sizeof(struct lcc_context));

    if( !newctx ){
        return NULL;
    }

    memset(newctx, 0, sizeof(struct lcc_context));

    newctx->flags = LCC_STATE_INHIBITED;

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

    frame.can_id = (0x7000 | ((ctx->unique_id & 0xFFF000000000l) >> 36)) << 12;
    frame.can_id |= ctx->node_alias | (0x01 << 27) | (0x01 << 28);
    ctx->write_function(&frame);

    frame.can_id = (0x6000 | ((ctx->unique_id & 0x000FFF000000l) >> 24)) << 12;
    frame.can_id |= ctx->node_alias | (0x01 << 27) | (0x01 << 28);
    ctx->write_function(&frame);

    frame.can_id = (0x5000 | ((ctx->unique_id & 0x000000FFF000l) >> 12)) << 12;
    frame.can_id |= ctx->node_alias | (0x01 << 27) | (0x01 << 28);
    ctx->write_function(&frame);

    frame.can_id = (0x4000 | ((ctx->unique_id & 0x000000000FFFl) >> 0)) << 12;
    frame.can_id |= ctx->node_alias | (0x01 << 27) | (0x01 << 28);
    ctx->write_function(&frame);

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
    ctx->write_function(&frame);

    // AMD frame
    lcc_set_lcb_variable_field(&frame, ctx, 0x701);
    frame.can_len = 6;
    frame.data[0] = (ctx->unique_id & 0xFF0000000000l) >> 40;
    frame.data[1] = (ctx->unique_id & 0x00FF00000000l) >> 32;
    frame.data[2] = (ctx->unique_id & 0x0000FF000000l) >> 24;
    frame.data[3] = (ctx->unique_id & 0x000000FF0000l) >> 16;
    frame.data[4] = (ctx->unique_id & 0x00000000FF00l) >> 8;
    frame.data[5] = (ctx->unique_id & 0x0000000000FFl) >> 0;
    ctx->write_function(&frame);

    return LCC_OK;
}
