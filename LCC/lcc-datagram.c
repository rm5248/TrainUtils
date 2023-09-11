/* SPDX-License-Identifier: GPL-2.0 */
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "lcc-datagram.h"
#include "lcc-common-internal.h"

static void lcc_datagram_append_bytes(struct lcc_datagram_buffer* datagram, void* bytes, int len){
    if(len < 0){
        return;
    }

    if((datagram->offset + len) > sizeof(datagram->buffer)){
        // We don't have enough room to add in this data
        return;
    }

    memcpy(datagram->buffer + datagram->offset, bytes, len);
    datagram->offset += len;
}

struct lcc_datagram_context* lcc_datagram_context_new(struct lcc_context* parent){
#ifdef ARDUINO
    static struct lcc_datagram_context ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.parent = parent;
    parent->datagram_context = &ctx;

    return &ctx;
#else
    struct lcc_datagram_context* ctx = malloc(sizeof(struct lcc_datagram_context));

    memset(ctx, 0, sizeof(struct lcc_datagram_context));
    ctx->parent = parent;
    parent->datagram_context = ctx;

    return ctx;
#endif
}

//void lcc_datagram_context_free(struct lcc_datagram_context* ctx){
//#ifdef ARDUINO
//#else
//    free(ctx);
//#endif
//}

struct lcc_context* lcc_datagram_context_parent(struct lcc_datagram_context* ctx){
    if(!ctx){
        return NULL;
    }

    return ctx->parent;
}

int lcc_datagram_context_set_datagram_functions(struct lcc_datagram_context* ctx,
                                       lcc_incoming_datagram_fn incoming_datagram,
                                       lcc_datagram_received_ok_fn datagram_ok,
                                                lcc_datagram_rejected_fn datagram_rejected){
    if(ctx == NULL){
        return LCC_ERROR_INVALID_ARG;
    }

    ctx->datagram_received_fn = incoming_datagram;
    ctx->datagram_ok_fn = datagram_ok;
    ctx->datagram_rejected_fn = datagram_rejected;

    return LCC_OK;
}

int lcc_datagram_load_and_send(struct lcc_datagram_context* ctx,
                               uint16_t alias,
                               void* data,
                               int data_len){
    struct lcc_can_frame frame;

    if(ctx == NULL || data_len < 0 || data_len > 72){
        return LCC_ERROR_INVALID_ARG;
    }

    if(data_len <= 8){
        // There's just one frame for us to send, so let's send it.
        lcc_set_lcb_variable_field(&frame, ctx->parent, alias);
        lcc_set_lcb_can_frame_type(&frame, 2);
        memcpy(frame.data, data, data_len);
        frame.can_len = data_len;

        return ctx->parent->write_function(ctx->parent, &frame);
    }

    int data_offset = 0;
    int numPackets = data_len / 8;
    if(data_len % 8 != 0){
        numPackets++;
    }
    for(int x = 0; x < numPackets; x++){
        lcc_set_lcb_variable_field(&frame, ctx->parent, alias);
        if(x == 0){
            // First frame
            lcc_set_lcb_can_frame_type(&frame, 3);
        }else if(x == numPackets - 1){
            // Last frame
            lcc_set_lcb_can_frame_type(&frame, 5);
        }else{
            // middle frame
            lcc_set_lcb_can_frame_type(&frame, 4);
        }

        int numBytesToCopy = (data_len - data_offset) >= 8 ? 8 : data_len - data_offset;
        memcpy(&frame.data, (uint8_t*)data + data_offset, numBytesToCopy);
        data_offset += numBytesToCopy;
        frame.can_len = numBytesToCopy;

        int write_ret = ctx->parent->write_function(ctx->parent, &frame);
        if(write_ret != LCC_OK){
            return write_ret;
        }
    }

    return LCC_OK;
}


int lcc_handle_datagram(struct lcc_context* ctx, struct lcc_can_frame* frame){
    int can_frame_type = (frame->can_id & LCC_CAN_FRAME_TYPE_MASK) >> 24;
    uint16_t mti = (frame->can_id & LCC_VARIABLE_FIELD_MASK) >> 12;
    uint16_t source_alias = (frame->can_id & LCC_NID_ALIAS_MASK);
    struct lcc_datagram_context* datagram_ctx = ctx->datagram_context;

    if(!datagram_ctx){
        return LCC_OK;
    }

    // See if this is a datagram frame
    if(can_frame_type == 2 ||
            can_frame_type == 3 ||
            can_frame_type == 4 ||
            can_frame_type == 5){
        // Check to see if this comes to us
        if(mti != ctx->node_alias){
            return LCC_OK;
        }

        // This is a datagram that is destined for us, append to our buffer
        lcc_datagram_append_bytes(&datagram_ctx->datagram_buffer, frame->data, frame->can_len);

        if((can_frame_type == 5 || can_frame_type == 2)){
            // This is the last frame - call our callback function.
            // First we check to see if we handle it within the library(CDI).
            // If we don't handle it within the library, call the callback function.
            int handled = 0;
            if(ctx->memory_context){
                handled = lcc_memory_try_handle_datagram(ctx->memory_context, source_alias, datagram_ctx->datagram_buffer.buffer, datagram_ctx->datagram_buffer.offset);
            }
            if(!handled && datagram_ctx->datagram_received_fn){
                datagram_ctx->datagram_received_fn(datagram_ctx, source_alias, datagram_ctx->datagram_buffer.buffer, datagram_ctx->datagram_buffer.offset);
            }else if(!handled){
                // There is no handler for this, reject it!
                lcc_datagram_respond_rejected(datagram_ctx, source_alias, 0, NULL);
            }

            // We have received a datagram, reset our buffer
            datagram_ctx->datagram_buffer.offset = 0;
        }
    }

    if(mti == LCC_MTI_DATAGRAM_RECEIVED_OK){
        if(datagram_ctx->datagram_received_fn){
            datagram_ctx->datagram_ok_fn(datagram_ctx, source_alias, frame->data[0]);
        }
    }else if(mti == LCC_MTI_DATAGRAM_REJECTED){
        if(datagram_ctx->datagram_rejected_fn){
            datagram_ctx->datagram_rejected_fn(datagram_ctx, source_alias, frame->data[2] << 8 | frame->data[3], NULL, 0);
        }
    }

    return LCC_OK;
}

int lcc_datagram_respond_rxok(struct lcc_datagram_context* ctx,
                              uint16_t alias,
                              int flags){

    // tell the sending node that we received OK
    struct lcc_can_frame frame;
    memset(&frame, 0, sizeof(struct lcc_can_frame));

    lcc_set_lcb_variable_field(&frame, ctx->parent, LCC_MTI_DATAGRAM_RECEIVED_OK);
    lcc_set_lcb_can_frame_type(&frame, 1);
    lcc_set_flags_and_dest_alias(&frame, LCC_FLAG_FRAME_ONLY, alias);
    frame.can_len = 3;
    frame.data[2] = flags;

    return ctx->parent->write_function(ctx->parent, &frame);
}

int lcc_datagram_respond_rejected(struct lcc_datagram_context* ctx,
                                  uint16_t alias,
                                  uint16_t error_code,
                                  const char* optional_info){
    // tell the sending node that we rejected the datagram
    struct lcc_can_frame frame;
    memset(&frame, 0, sizeof(struct lcc_can_frame));

    lcc_set_lcb_variable_field(&frame, ctx->parent, LCC_MTI_DATAGRAM_REJECTED);
    lcc_set_lcb_can_frame_type(&frame, 1);
    lcc_set_flags_and_dest_alias(&frame, LCC_FLAG_FRAME_ONLY, alias);
    frame.can_len = 2;
    frame.data[0] = (error_code & 0xFF00) >> 8;
    frame.data[1] = error_code & 0xFF;

    return ctx->parent->write_function(ctx->parent, &frame);
}
