/* SPDX-License-Identifier: GPL-2.0 */
#include <stdlib.h>
#include <string.h>
#include "lcc-remote-memory.h"
#include "lcc-common-internal.h"
#include "lcc-memory.h"
#include "lcc-datagram.h"

struct lcc_remote_memory_context* lcc_remote_memory_new(struct lcc_context* ctx){
    if(ctx->datagram_context == NULL){
        return NULL;
    }

    if(ctx->remote_memory_context){
        return ctx->remote_memory_context;
    }

#ifdef LIBLCC_ENABLE_STATIC_CONTEXT
    static struct lcc_remote_memory_context mem_ctx;
    memset(&mem_ctx, 0, sizeof(mem_ctx));
    mem_ctx.parent = ctx;
    ctx->memory_context = &mem_ctx;

    return &mem_ctx;
#else
    struct lcc_remote_memory_context* memory = malloc(sizeof(struct lcc_remote_memory_context));

    memset(memory, 0, sizeof(struct lcc_remote_memory_context));
    memory->parent = ctx;
    ctx->remote_memory_context = memory;

    return memory;
#endif
}

int lcc_remote_memory_read_single_transfer(struct lcc_remote_memory_context* ctx, int alias, uint8_t space, uint32_t starting_address, int read_count){
    if(ctx->parent->datagram_context == NULL){
        return LCC_ERROR_NO_DATAGRAM_HANDLING;
    }

    if(ctx->current_requesting_alias != 0){
        // Only one memory request can be outstanding at a time
        return LCC_ERROR_MEMORY_TX_IN_PROGRESS;
    }

    struct lcc_datagram_context* datagram_ctx = ctx->parent->datagram_context;
    if(lcc_datagram_transfer_in_progress(ctx->parent->datagram_context)){
        return LCC_ERROR_DATAGRAM_IN_PROGRESS;
    }

    ctx->current_requesting_alias = alias;
    memset(&datagram_ctx->datagram_buffer, 0, sizeof(datagram_ctx->datagram_buffer));

    if(read_count > 64 || read_count < 0){
        return LCC_ERROR_INVALID_ARG;
    }

    uint8_t tx_data[8];
    tx_data[0] = 0x20;
    tx_data[1] = (0x01 << 6); /* read operation */
    if(space == LCC_MEMORY_SPACE_CONFIGURATION_DEFINITION){
        tx_data[1] |= 0x3;
    }else if(space == LCC_MEMORY_SPACE_ALL_MEMORY){
        tx_data[1] |= 0x2;
    }else if(space == LCC_MEMORY_SPACE_CONFIGURATION_SPACE){
        tx_data[1] |= 0x01;
    }else{
        tx_data[6] = space;
    }
    lcc_uint32_to_data(tx_data + 2, starting_address);

    tx_data[6] = read_count & 0xFF;

    return lcc_datagram_load_and_send(ctx->parent->datagram_context,
                               alias,
                               tx_data, 7);
}

int lcc_remote_memory_get_address_space_information(struct lcc_remote_memory_context* ctx, int alias, uint8_t space){
    if(ctx == NULL){
        return LCC_ERROR_INVALID_ARG;
    }

    uint8_t tx_data[8];
    tx_data[0] = 0x20;
    tx_data[1] = 0x84;
    tx_data[2] = space;

    return lcc_datagram_load_and_send(ctx->parent->datagram_context,
                               alias,
                               tx_data, 3);
}


int lcc_remote_memory_try_handle_datagram(struct lcc_remote_memory_context* ctx, uint16_t alias, uint8_t* data, int data_len){
    if(data_len < 2 || data[0] != 0x20){
        return 0;
    }

    uint8_t address_space = 0;
    uint32_t starting_address = lcc_uint32_from_data(data + 2);
    int is_failure = 0;
    int data_starting_byte = 6;
    uint16_t current_request_alias = ctx->current_requesting_alias;

    if(alias != current_request_alias){
        return 0;
    }

    if(data[1] == 0x50){
        address_space = data[6];
    }else if(data[1] == 0x51){
        address_space = LCC_MEMORY_SPACE_CONFIGURATION_SPACE;
    }else if(data[1] == 0x52){
        address_space = LCC_MEMORY_SPACE_ALL_MEMORY;
    }else if(data[1] == 0x53){
        address_space = LCC_MEMORY_SPACE_CONFIGURATION_DEFINITION;
    }else if(data[1] == 0x58){
        is_failure = 1;
        data_starting_byte = 7;
        address_space = data[6];
    }else if(data[1] == 0x59){
        is_failure = 1;
        address_space = LCC_MEMORY_SPACE_CONFIGURATION_SPACE;
    }else if(data[1] == 0x5A){
        is_failure = 1;
        address_space = LCC_MEMORY_SPACE_ALL_MEMORY;
    }else if(data[1] == 0x5B){
        is_failure = 1;
        address_space = LCC_MEMORY_SPACE_CONFIGURATION_DEFINITION;
    }else{
        // We did not handle this - it could be for something else
        return 0;
    }

    ctx->current_requesting_alias = 0;

    if(is_failure){
        uint16_t error_code = (data[data_starting_byte] << 8) | data[data_starting_byte + 1];
        const char* message_string = NULL;

        if(data_len > data_starting_byte){
            message_string = data[data_starting_byte + 2];
        }

        if(ctx->read_rejected){
            ctx->read_rejected(ctx, alias, address_space, starting_address, error_code, message_string);
        }

        return 1;
    }else{
        if(ctx->remote_memory_received){
            ctx->remote_memory_received(ctx, alias, address_space, starting_address, data + data_starting_byte, data_len - data_starting_byte);
        }
    }

    return 1;
}

int lcc_remote_memory_set_functions(struct lcc_remote_memory_context* ctx,
                                    lcc_remote_memory_request_ok remote_ok,
                                    lcc_remote_memory_request_fail remote_fail,
                                    lcc_remote_memory_received received_memory,
                                    lcc_remote_memory_read_rejected read_rejected ){
    if(ctx == NULL){
        return LCC_ERRCODE_INVALID_ARG;
    }

    ctx->remote_request_ok = remote_ok;
    ctx->remote_request_fail = remote_fail;
    ctx->remote_memory_received = received_memory;
    ctx->read_rejected = read_rejected;

    return LCC_OK;
}

int lcc_remote_memory_handle_datagram_rx_ok(struct lcc_remote_memory_context* ctx, uint16_t alias, uint8_t flags){
    if(alias != ctx->current_requesting_alias){
        return 0;
    }

    if(ctx->remote_request_ok){
        ctx->remote_request_ok(ctx, alias, flags);
    }

    return 1;
}

int lcc_remote_memory_handle_datagram_rejected(struct lcc_remote_memory_context* ctx, uint16_t alias, uint16_t error_code, void* optional_data, int optional_len){
    if(alias != ctx->current_requesting_alias){
        return 0;
    }

    ctx->current_requesting_alias = 0;
    if(ctx->remote_request_fail){
        ctx->remote_request_fail(ctx, alias, error_code, optional_data, optional_len);
    }

    return 1;
}

uint16_t lcc_remote_memory_current_alias_request(struct lcc_remote_memory_context* ctx){
    if(ctx == NULL){
        return 0;
    }
    return ctx->current_requesting_alias;
}

struct lcc_context* lcc_remote_memory_parent(struct lcc_remote_memory_context* ctx){
    if(ctx == NULL){
        return NULL;
    }

    return ctx->parent;
}
