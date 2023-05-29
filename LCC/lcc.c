/* SPDX-License-Identifier: GPL-2.0 */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "lcc.h"
#include "lcc-common-internal.h"
#include "lcc-common.h"
#include "lcc-simple.h"
#include "lcc-addressed.h"

static int is_datagram_frame(struct lcc_can_frame* frame){
    int can_frame_type = (frame->can_id & LCC_CAN_FRAME_TYPE_MASK) >> 24;
    uint16_t mti = (frame->can_id & LCC_VARIABLE_FIELD_MASK) >> 12;

    if(can_frame_type == 2 ||
            can_frame_type == 3 ||
            can_frame_type == 4 ||
            can_frame_type == 5){
        return 1;
    }

    if(mti == LCC_MTI_DATAGRAM_RECEIVED_OK ||
            mti == LCC_MTI_DATAGRAM_REJECTED){
        return 1;
    }

    return 0;
}

struct lcc_context* lcc_context_new(){
#ifdef ARDUINO
    static struct lcc_context ctx;
    memset(&ctx, 0, sizeof(struct lcc_context));

    ctx.state = LCC_STATE_INHIBITED;
    return &ctx;
#else
    struct lcc_context* newctx = malloc(sizeof(struct lcc_context));

    if( !newctx ){
        return NULL;
    }

    memset(newctx, 0, sizeof(struct lcc_context));

    newctx->state = LCC_STATE_INHIBITED;

    return newctx;
#endif
}

void lcc_context_free(struct lcc_context* ctx){
    if( !ctx ){
        return;
    }

#ifndef ARDUINO
    if(ctx->datagram_context){
        free(ctx->datagram_context);
    }
    if(ctx->memory_context){
        free(ctx->memory_context);
    }
    free(ctx);
#endif
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
    int can_frame_type = (frame->can_id & LCC_CAN_FRAME_TYPE_MASK) >> 24;

    // TODO the handlers below should probably be in a list of some kind,
    // so that we just call them sequentially until somebody can handle the request
    if(is_datagram_frame(frame)){
        return lcc_handle_datagram(ctx, frame);
    }

    if(mti & LCC_MTI_SIMPLE){
        // this is a simple message
        return lcc_handle_simple_protocol(ctx, frame);
    }

    if(mti & LCC_MTI_ADDRESSED){
        // This is an addressed message.  If it is for us, we will attempt
        // to handle it
        return lcc_handle_addressed(ctx, frame);
    }

    if(frame_type != 1){
        return LCC_OK;
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

    lcc_set_lcb_variable_field(&frame, ctx, ((ctx->unique_id & 0xFFF000000000llu) >> 36) | 0x7000);
    // this is a CAN control frame, clear bit 27
    frame.can_id &= (~(0x01l << 27));
    ctx->write_function(ctx, &frame);

    lcc_set_lcb_variable_field(&frame, ctx, ((ctx->unique_id & 0x000FFF000000llu) >> 24) | 0x6000);
    frame.can_id &= (~(0x01l << 27));
    ctx->write_function(ctx, &frame);

    lcc_set_lcb_variable_field(&frame, ctx, ((ctx->unique_id & 0x000000FFF000llu) >> 12) | 0x5000);
    frame.can_id &= (~(0x01l << 27));
    ctx->write_function(ctx, &frame);

    lcc_set_lcb_variable_field(&frame, ctx, ((ctx->unique_id & 0x000000000FFFllu) >> 0) | 0x4000);
    frame.can_id &= (~(0x01l << 27));
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
    frame.can_id &= (~(0x01l << 27));
    ctx->write_function(ctx, &frame);

    // AMD frame
    lcc_set_lcb_variable_field(&frame, ctx, 0x701);
    frame.can_id &= (~(0x01l << 27));
    lcc_set_nodeid_in_data(&frame, ctx->unique_id);
    ctx->write_function(ctx, &frame);

    // Send 'initialization complete' frame
    lcc_set_lcb_variable_field(&frame, ctx, 0x100);
    lcc_set_lcb_can_frame_type(&frame, 1);
    lcc_set_nodeid_in_data(&frame, ctx->unique_id);
    ctx->write_function(ctx, &frame);

    // Send out our list of events that we produce
    lcc_send_events_produced(ctx);

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

int lcc_context_set_simple_node_information(struct lcc_context* ctx,
                                            const char* manufacturer_name,
                                            const char* model_name,
                                            const char* hw_version,
                                            const char* sw_version){
    if( !ctx ){
        return LCC_ERROR_INVALID_ARG;
    }

    int manufacturer_string_len = strlen(manufacturer_name);
    int model_string_len = strlen(model_name);
    int hardware_string_len =strlen(hw_version);
    int software_string_len = strlen(sw_version);

    if(strlen(manufacturer_name) > 40 ||
            strlen(model_name) > 40 ||
            strlen(hw_version) > 20 ||
            strlen(sw_version) > 20){
        return LCC_ERROR_STRING_TOO_LONG;
    }

    memcpy(ctx->simple_info.manufacturer_name, manufacturer_name, manufacturer_string_len + 1);
    memcpy(ctx->simple_info.model_name, model_name, model_string_len + 1);
    memcpy(ctx->simple_info.hw_version, hw_version, hardware_string_len + 1);
    memcpy(ctx->simple_info.sw_version, sw_version, software_string_len + 1);

    return LCC_OK;
}

int lcc_context_set_simple_node_name_description(struct lcc_context* ctx,
                                                 const char* node_name,
                                                 const char* node_description){
    if( !ctx ){
        return LCC_ERROR_INVALID_ARG;
    }

    int node_name_len = strlen(node_name);
    int node_description_len = strlen(node_description);

    if(node_name_len > 62 ||
            node_description_len > 63){
        return LCC_ERROR_STRING_TOO_LONG;
    }

    memcpy(ctx->simple_info.node_name, node_name, node_name_len + 1);
    memcpy(ctx->simple_info.node_description, node_description, node_description_len + 1);

    return LCC_OK;
}

int lcc_node_id_to_dotted_format(uint64_t node_id, char* buffer, int buffer_len){
    if(buffer_len < 20){
        return LCC_ERROR_BUFFER_SIZE_INCORRECT;
    }

    snprintf(buffer, buffer_len, "%02X.%02X.%02X.%02X.%02X.%02X",
             (int)((node_id & 0xFF0000000000l) >> 40) & 0xFF,
             (int)((node_id & 0x00FF00000000l) >> 32) & 0xFF,
             (int)((node_id & 0x0000FF000000l) >> 24) & 0xFF,
             (int)((node_id & 0x000000FF0000l) >> 16) & 0xFF,
             (int)((node_id & 0x00000000FF00l) >> 8) & 0xFF,
             (int)((node_id & 0x0000000000FFl) >> 0) & 0xFF);

    return LCC_OK;
}

int lcc_context_current_state(struct lcc_context* ctx){
    if(!ctx){
        return LCC_ERROR_INVALID_ARG;
    }

    if(ctx->state){
        return LCC_STATE_INHIBITED;
    }

    return LCC_STATE_PERMITTED;
}


struct lcc_datagram_context* lcc_context_get_datagram_context(struct lcc_context* ctx){
    if(!ctx){
        return NULL;
    }

    return ctx->datagram_context;
}

struct lcc_memory_context* lcc_context_get_memory_context(struct lcc_context* ctx){
    if(!ctx){
        return NULL;
    }

    return ctx->memory_context;
}

struct lcc_event_context* lcc_context_get_event_context(struct lcc_context* ctx){
    if(!ctx){
        return NULL;
    }

    return ctx->event_context;
}

