/* SPDX-License-Identifier: GPL-2.0 */
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "lcc-common-internal.h"
#include "lcc-memory.h"
#include "lcc-datagram.h"

struct lcc_firmware_upgrade_context* lcc_firmware_upgrade_new(struct lcc_context* parent){
#ifdef LIBLCC_ENABLE_STATIC_CONTEXT
    static struct lcc_firmware_upgrade_context fw_upgrade_ctx;
    memset(&fw_upgrade_ctx, 0, sizeof(fw_upgrade_ctx));
    fw_upgrade_ctx.parent = parent;
    parent->firmware_upgrade_context = &fw_upgrade_ctx;

    return &fw_upgrade_ctx;
#else
    struct lcc_firmware_upgrade_context* fw_upgrade_ctx = malloc(sizeof(struct lcc_firmware_upgrade_context));

    memset(fw_upgrade_ctx, 0, sizeof(struct lcc_firmware_upgrade_context));
    fw_upgrade_ctx->parent = parent;
    parent->firmware_upgrade_context = fw_upgrade_ctx;

    return fw_upgrade_ctx;
#endif
}

int lcc_firmware_upgrade_in_progress(struct lcc_firmware_upgrade_context* ctx){
    if(ctx == NULL){
        return LCC_ERROR_INVALID_ARG;
    }

    return ctx->upgrade_in_progress;
}

int _lcc_firmware_upgrade_freeze(struct lcc_firmware_upgrade_context* ctx, uint16_t alias){
    ctx->upgrade_in_progress = 1;
    if(lcc_datagram_respond_rxok(ctx->parent->datagram_context, alias, 0) != LCC_OK){
        return 0;
    }
    // Now send the 'Node Initialization Complete' message
    struct lcc_can_frame frame;
    memset(&frame, 0, sizeof(frame));
    lcc_set_lcb_variable_field(&frame, ctx->parent, 0x100);
    lcc_set_lcb_can_frame_type(&frame, 1);
    lcc_set_nodeid_in_data(&frame, ctx->parent->unique_id);
    ctx->parent->write_function(ctx->parent, &frame);

    if(ctx->start_fn){
        ctx->start_fn(ctx);
    }

    return 0;
}

int _lcc_firmware_upgrade_unfreeze(struct lcc_firmware_upgrade_context* ctx, uint16_t alias){
    ctx->upgrade_in_progress = 0;
    if(lcc_datagram_respond_rxok(ctx->parent->datagram_context, alias, 0) != LCC_OK){
        return 0;
    }
    // Now send the 'Node Initialization Complete' message
    struct lcc_can_frame frame;
    memset(&frame, 0, sizeof(frame));
    lcc_set_lcb_variable_field(&frame, ctx->parent, 0x100);
    lcc_set_lcb_can_frame_type(&frame, 1);
    lcc_set_nodeid_in_data(&frame, ctx->parent->unique_id);
    ctx->parent->write_function(ctx->parent, &frame);

    if(ctx->finished_fn){
        ctx->finished_fn(ctx);
    }

    return 0;
}


int _lcc_firmware_upgrade_incoming_write(struct lcc_firmware_upgrade_context* ctx, uint16_t alias, uint32_t starting_address, uint8_t* data, int data_len){
    // We have some incoming data for the firmware upgrade.  Let's handle it!
    if(ctx->write_fn == NULL){
        return -1;
    }

    ctx->addr = starting_address;
    ctx->alias = alias;
    ctx->calling_write_cb = 1;
    ctx->write_fn(ctx, starting_address, data, data_len);
    ctx->calling_write_cb = 0;

    return 0;
}

int lcc_firmware_upgrade_set_functions(struct lcc_firmware_upgrade_context* ctx,
                                       lcc_firmware_upgrade_start start_fn,
                                       lcc_firmware_upgrade_incoming_data incoming_fn,
                                       lcc_firmware_upgrade_finished finished_fn){
    if(!ctx || !start_fn ||  !incoming_fn || !finished_fn){
        return LCC_ERROR_INVALID_ARG;
    }

    ctx->start_fn = start_fn;
    ctx->write_fn = incoming_fn;
    ctx->finished_fn = finished_fn;

    return LCC_OK;
}

int lcc_firmware_write_ok(struct lcc_firmware_upgrade_context* ctx){
    if(ctx == NULL){
        return LCC_ERROR_INVALID_ARG;
    }

    if(ctx->calling_write_cb == 0){
        return LCC_ERROR_INVALID_PROGRAM_STATE;
    }

    return lcc_memory_respond_write_reply_ok(ctx->parent->memory_context,
                                             ctx->alias,
                                             LCC_MEMORY_SPACE_FIRMWARE,
                                             ctx->addr);
}

int lcc_firmware_write_error(struct lcc_firmware_upgrade_context* ctx,
                             uint16_t error_code,
                             const char* optional_info){
    if(ctx == NULL){
        return LCC_ERROR_INVALID_ARG;
    }

    if(ctx->calling_write_cb == 0){
        return LCC_ERROR_INVALID_PROGRAM_STATE;
    }

    return lcc_memory_respond_write_reply_fail(ctx->parent->memory_context,
                                             ctx->alias,
                                             LCC_MEMORY_SPACE_FIRMWARE,
                                             ctx->addr,
                                               error_code,
                                               optional_info);
}

