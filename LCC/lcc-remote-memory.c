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

    if(read_count > 64 || read_count < 0){
        return LCC_ERROR_INVALID_ARG;
    }

    ctx->current_requesting_alias = alias;
    ctx->current_request_is_write = 0;
    memset(&datagram_ctx->datagram_buffer, 0, sizeof(datagram_ctx->datagram_buffer));

    uint8_t tx_data[8];
    int count_byte = 6;
    tx_data[0] = 0x20;
    tx_data[1] = (0x01 << 6); /* read operation */
    if(space == LCC_MEMORY_SPACE_CONFIGURATION_DEFINITION){
        tx_data[1] |= 0x3;
    }else if(space == LCC_MEMORY_SPACE_ALL_MEMORY){
        tx_data[1] |= 0x2;
    }else if(space == LCC_MEMORY_SPACE_CONFIGURATION_SPACE){
        tx_data[1] |= 0x01;
    }else{
        /* Generic space: the space goes in byte 6, and the count after it */
        tx_data[6] = space;
        count_byte = 7;
    }
    lcc_uint32_to_data(tx_data + 2, starting_address);

    tx_data[count_byte] = read_count & 0xFF;

    return lcc_datagram_load_and_send(ctx->parent->datagram_context,
                               alias,
                               tx_data, count_byte + 1);
}

int lcc_remote_memory_write_single_transfer(struct lcc_remote_memory_context* ctx, int alias, uint8_t space, uint32_t starting_address, const void* data, int data_len){
    if(ctx == NULL || data == NULL || data_len < 1 || data_len > 64){
        return LCC_ERROR_INVALID_ARG;
    }

    if(ctx->parent->datagram_context == NULL){
        return LCC_ERROR_NO_DATAGRAM_HANDLING;
    }

    if(ctx->current_requesting_alias != 0){
        // Only one memory request can be outstanding at a time
        return LCC_ERROR_MEMORY_TX_IN_PROGRESS;
    }

    if(lcc_datagram_transfer_in_progress(ctx->parent->datagram_context)){
        return LCC_ERROR_DATAGRAM_IN_PROGRESS;
    }

    uint8_t tx_data[72];
    int data_offset = 6;
    tx_data[0] = 0x20;
    tx_data[1] = 0x00; /* write operation */
    if(space == LCC_MEMORY_SPACE_CONFIGURATION_DEFINITION){
        tx_data[1] |= 0x3;
    }else if(space == LCC_MEMORY_SPACE_ALL_MEMORY){
        tx_data[1] |= 0x2;
    }else if(space == LCC_MEMORY_SPACE_CONFIGURATION_SPACE){
        tx_data[1] |= 0x01;
    }else{
        /* Generic space: the space goes in byte 6, and the data after it */
        tx_data[6] = space;
        data_offset = 7;
    }
    lcc_uint32_to_data(tx_data + 2, starting_address);
    memcpy(tx_data + data_offset, data, data_len);

    ctx->current_requesting_alias = alias;
    ctx->current_request_is_write = 1;
    ctx->current_write_space = space;
    ctx->current_write_address = starting_address;

    return lcc_datagram_load_and_send(ctx->parent->datagram_context,
                               alias,
                               tx_data, data_offset + data_len);
}

int lcc_remote_memory_get_address_space_information(struct lcc_remote_memory_context* ctx, int alias, uint8_t space){
    if(ctx == NULL){
        return LCC_ERROR_INVALID_ARG;
    }

    uint8_t tx_data[8];
    tx_data[0] = 0x20;
    tx_data[1] = 0x84;
    tx_data[2] = space;
    ctx->current_requesting_alias = alias;
    ctx->current_request_is_write = 0;

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
    int is_write_reply = 0;
    int data_starting_byte = 6;
    uint16_t current_request_alias = ctx->current_requesting_alias;
    int is_response = 0;

    if(alias != current_request_alias){
        return 0;
    }

    if(data[1] == 0x50){
        address_space = data[6];
        data_starting_byte = 7;
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
    }else if(data[1] == 0x10){
        is_write_reply = 1;
        address_space = data[6];
    }else if(data[1] == 0x11){
        is_write_reply = 1;
        address_space = LCC_MEMORY_SPACE_CONFIGURATION_SPACE;
    }else if(data[1] == 0x12){
        is_write_reply = 1;
        address_space = LCC_MEMORY_SPACE_ALL_MEMORY;
    }else if(data[1] == 0x13){
        is_write_reply = 1;
        address_space = LCC_MEMORY_SPACE_CONFIGURATION_DEFINITION;
    }else if(data[1] == 0x18){
        is_write_reply = 1;
        is_failure = 1;
        data_starting_byte = 7;
        address_space = data[6];
    }else if(data[1] == 0x19){
        is_write_reply = 1;
        is_failure = 1;
        address_space = LCC_MEMORY_SPACE_CONFIGURATION_SPACE;
    }else if(data[1] == 0x1A){
        is_write_reply = 1;
        is_failure = 1;
        address_space = LCC_MEMORY_SPACE_ALL_MEMORY;
    }else if(data[1] == 0x1B){
        is_write_reply = 1;
        is_failure = 1;
        address_space = LCC_MEMORY_SPACE_CONFIGURATION_DEFINITION;
    }else if(data[1] == 0x86 || data[1] == 0x87){
        is_response = 1;
    }else{
        // We did not handle this - it could be for something else
        return 0;
    }

    ctx->current_requesting_alias = 0;
    ctx->current_request_is_write = 0;

    if(is_failure){
        uint16_t error_code = 0;
        // The message is optional, and is not necessarily NULL terminated in the datagram
        char message_buffer[72];
        const char* message_string = NULL;

        if(data_len >= data_starting_byte + 2){
            error_code = (data[data_starting_byte] << 8) | data[data_starting_byte + 1];
        }
        if(data_len > data_starting_byte + 2){
            int message_len = data_len - (data_starting_byte + 2);
            memcpy(message_buffer, data + data_starting_byte + 2, message_len);
            message_buffer[message_len] = 0;
            message_string = message_buffer;
        }

        if(is_write_reply){
            if(ctx->write_rejected){
                ctx->write_rejected(ctx, alias, address_space, starting_address, error_code, message_string);
            }
        }else if(ctx->read_rejected){
            ctx->read_rejected(ctx, alias, address_space, starting_address, error_code, message_string);
        }
        lcc_datagram_respond_rxok(ctx->parent->datagram_context, alias, 0);
    }else if(is_write_reply){
        if(ctx->write_ok){
            ctx->write_ok(ctx, alias, address_space, starting_address);
        }
        lcc_datagram_respond_rxok(ctx->parent->datagram_context, alias, 0);
    }else if(is_response){
        int present = data[1] == 0x87; // 0x87 = present, 0x86 = not present
        uint8_t address_space = data[2];
        uint32_t high_address = lcc_uint32_from_data(data + 3);
        uint32_t low_address = 0;
        int readonly = 0;

        if(data[7] & (0x01 << 1) && data_len >= 12){
            // if bit 1 is set, low address is in next 4 bytes
            low_address = lcc_uint32_from_data(data + 8);
        }
        if(data[7] & 0x01){
            readonly = 1;
        }

        if(ctx->remote_memory_info){
            ctx->remote_memory_info(ctx, alias, present, readonly, address_space, low_address, high_address, NULL);
        }
        lcc_datagram_respond_rxok(ctx->parent->datagram_context, alias, 0);
    }else{
        if(ctx->remote_memory_received){
            ctx->remote_memory_received(ctx, alias, address_space, starting_address, data + data_starting_byte, data_len - data_starting_byte);
        }
        lcc_datagram_respond_rxok(ctx->parent->datagram_context, alias, 0);
    }

    return 1;
}

int lcc_remote_memory_set_functions(struct lcc_remote_memory_context* ctx,
                                    lcc_remote_memory_request_ok remote_ok,
                                    lcc_remote_memory_request_fail remote_fail,
                                    lcc_remote_memory_received received_memory,
                                    lcc_remote_memory_read_rejected read_rejected,
                                    lcc_remote_memory_information remote_info_response){
    if(ctx == NULL){
        return LCC_ERRCODE_INVALID_ARG;
    }

    ctx->remote_request_ok = remote_ok;
    ctx->remote_request_fail = remote_fail;
    ctx->remote_memory_received = received_memory;
    ctx->read_rejected = read_rejected;
    ctx->remote_memory_info = remote_info_response;

    return LCC_OK;
}

int lcc_remote_memory_set_write_functions(struct lcc_remote_memory_context* ctx,
                                          lcc_remote_memory_write_ok write_ok,
                                          lcc_remote_memory_write_rejected write_rejected){
    if(ctx == NULL){
        return LCC_ERRCODE_INVALID_ARG;
    }

    ctx->write_ok = write_ok;
    ctx->write_rejected = write_rejected;

    return LCC_OK;
}

int lcc_remote_memory_handle_datagram_rx_ok(struct lcc_remote_memory_context* ctx, uint16_t alias, uint8_t flags){
    if(alias != ctx->current_requesting_alias){
        return 0;
    }

    if(ctx->remote_request_ok){
        ctx->remote_request_ok(ctx, alias, flags);
    }

    if(ctx->current_request_is_write && !(flags & LCC_DATAGRAM_REPLY_PENDING)){
        // The node accepted the write without a reply pending, so the write is complete
        uint8_t space = ctx->current_write_space;
        uint32_t address = ctx->current_write_address;
        ctx->current_requesting_alias = 0;
        ctx->current_request_is_write = 0;
        if(ctx->write_ok){
            ctx->write_ok(ctx, alias, space, address);
        }
    }

    return 1;
}

int lcc_remote_memory_handle_datagram_rejected(struct lcc_remote_memory_context* ctx, uint16_t alias, uint16_t error_code, void* optional_data, int optional_len){
    if(alias != ctx->current_requesting_alias){
        return 0;
    }

    ctx->current_requesting_alias = 0;
    ctx->current_request_is_write = 0;
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
