/* SPDX-License-Identifier: GPL-2.0 */
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>

#include "lcc-memory.h"
#include "lcc-common-internal.h"
#include "lcc-datagram.h"

#ifdef ARDUINO
#include <avr/pgmspace.h>

static uint8_t lcc_memory_byte_from_data(void* data, int offset, int flags){
    uint8_t* u8_data = data;
    if(flags & LCC_MEMORY_CDI_FLAG_ARDUINO_PROGMEM){
        return pgm_read_byte(u8_data + offset);
    }else{
        return u8_data[offset];
    }
}
#else
static uint8_t lcc_memory_byte_from_data(void* data, int offset, int flags){
    uint8_t* u8_data = data;
    return u8_data[offset];
}
#endif /* ARDUINO */

struct lcc_memory_context* lcc_memory_new(struct lcc_context* ctx){
    if(ctx->datagram_context == NULL){
        return NULL;
    }

#ifdef ARDUINO
    static struct lcc_memory_context mem_ctx;
    memset(&mem_ctx, 0, sizeof(mem_ctx));
    mem_ctx.parent = ctx;
    ctx->memory_context = &mem_ctx;

    return &mem_ctx;
#else
    struct lcc_memory_context* memory = malloc(sizeof(struct lcc_memory_context));

    memset(memory, 0, sizeof(struct lcc_memory_context));
    memory->parent = ctx;
    ctx->memory_context = memory;

    return memory;
#endif
}

int lcc_memory_read_single_transfer(struct lcc_context* ctx, int alias, uint8_t space, uint32_t starting_address, int read_count){
    if(ctx->datagram_context == NULL){
        return LCC_ERROR_NO_DATAGRAM_HANDLING;
    }

    memset(&ctx->datagram_context->datagram_buffer, 0, sizeof(ctx->datagram_context->datagram_buffer));

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

    lcc_datagram_load_and_send(ctx->datagram_context,
                               alias,
                               tx_data, 7);

//    struct lcc_can_frame frame;
//    memset(&frame, 0, sizeof(frame));

//    lcc_set_lcb_variable_field(&frame, ctx, alias);
//    lcc_set_lcb_can_frame_type(&frame, 1);

//    // The frame format is 0x1Adddsss
//    int real_id = frame.can_id & 0xFFFFFF;
//    real_id |= 0x1A000000;
//    frame.can_id = real_id;

//    frame.can_len = 7;
//    frame.data[0] = 0x20;
//    frame.data[1] = (0x01 << 6); /* read operation */
//    if(space == LCC_MEMORY_SPACE_CONFIGURATION_DEFINITION){
//        frame.data[1] |= 0x3;
//    }else if(space == LCC_MEMORY_SPACE_ALL_MEMORY){
//        frame.data[1] |= 0x2;
//    }else if(space == LCC_MEMORY_SPACE_CONFIGURATION_SPACE){
//        frame.data[1] |= 0x01;
//    }else{
//        frame.data[6] = space;
//    }
//    frame.data[2] = ((starting_address & 0xFF000000) >> 24) & 0xFF;
//    frame.data[3] = ((starting_address & 0x00FF0000) >> 16) & 0xFF;
//    frame.data[4] = ((starting_address & 0x0000FF00) >> 8) & 0xFF;
//    frame.data[5] = ((starting_address & 0x000000FF) >> 0) & 0xFF;
//    frame.data[6] = read_count & 0xFF;
//    ctx->write_function(ctx, &frame);

    return LCC_OK;
}

int lcc_memory_get_address_space_information(struct lcc_context* ctx, int alias, uint8_t space){
    if(ctx == NULL){
        return LCC_ERROR_INVALID_ARG;
    }

    uint8_t tx_data[8];
    tx_data[0] = 0x20;
    tx_data[1] = 0x84;
    tx_data[2] = space;

    lcc_datagram_load_and_send(ctx->datagram_context,
                               alias,
                               tx_data, 3);

//    struct lcc_can_frame frame;
//    memset(&frame, 0, sizeof(frame));

//    lcc_set_lcb_variable_field(&frame, ctx, alias);
//    lcc_set_lcb_can_frame_type(&frame, 1);

//    // The frame format is 0x1Adddsss
//    int real_id = frame.can_id & 0xFFFFFF;
//    real_id |= 0x1A000000;
//    frame.can_id = real_id;

//    frame.can_len = 3;
//    frame.data[0] = 0x20;
//    frame.data[1] = 0x84;
//    frame.data[2] = space;
//    ctx->write_function(ctx, &frame);

    return LCC_OK;
}

int lcc_memory_set_cdi(struct lcc_memory_context* ctx, void* cdi_data, int cdi_len, int flags){
    if(ctx == NULL){
        return LCC_ERROR_INVALID_ARG;
    }

    ctx->cdi_data = cdi_data;
    ctx->cdi_flags = flags;
    ctx->cdi_length = cdi_len;

    return LCC_OK;
}

int lcc_memory_set_memory_functions(struct lcc_memory_context* ctx,
                                    lcc_address_space_information_query query_fn,
                                    lcc_address_space_read read_fn,
                                    lcc_address_space_write write_fn){
    if(ctx == NULL){
        return LCC_ERROR_INVALID_ARG;
    }

    ctx->query_fn = query_fn;
    ctx->read_fn = read_fn;
    ctx->write_fn = write_fn;

    return LCC_OK;
}

int lcc_memory_respond_information_query(struct lcc_memory_context* ctx,
                                         uint16_t alias,
                                          uint8_t address_space_present,
                                          uint8_t address_space,
                                          uint32_t highest_address,
                                          uint8_t flags,
                                         uint32_t lowest_address){
    if(ctx == NULL){
        return LCC_ERROR_INVALID_ARG;
    }

    uint8_t response[12];
    response[0] = 0x20;
    if(address_space_present){
        response[1] = 0x86;
    }else{
        response[1] = 0x87;
    }
    response[2] = address_space;
    lcc_uint32_to_data(response + 3, highest_address);
    response[7] = flags;
    lcc_uint32_to_data(response + 9, lowest_address);

    return lcc_datagram_load_and_send(ctx->parent->datagram_context,
                               alias,
                               response, 12);
}

int lcc_memory_respond_write_reply_ok(struct lcc_memory_context* ctx,
                                      uint16_t alias,
                                   uint8_t space,
                                      uint32_t starting_address){
    if(ctx == NULL){
        return LCC_ERROR_INVALID_ARG;
    }

    uint8_t response[12];
    response[0] = 0x20;
    int len = 6;
    if(space == LCC_MEMORY_SPACE_CONFIGURATION_SPACE){
        response[1] = 0x11;
    }else if(space == LCC_MEMORY_SPACE_ALL_MEMORY){
        response[1] = 0x12;
    }else if(space == LCC_MEMORY_SPACE_CONFIGURATION_DEFINITION){
        response[1] = 0x13;
    }else{
        response[1] = 0x10;
        response[6] = space;
        len = 7;
    }

    lcc_uint32_to_data(response + 2, starting_address);

    return lcc_datagram_load_and_send(ctx->parent->datagram_context,
                               alias,
                               response, len);
}

int lcc_memory_respond_write_reply_fail(struct lcc_memory_context* ctx,
                                        uint16_t alias,
                                   uint8_t space,
                                        uint32_t starting_address,
                                        uint16_t error_code,
                                        const char* message){
    if(ctx == NULL){
        return LCC_ERROR_INVALID_ARG;
    }

    uint8_t response[72];
    response[0] = 0x20;
    int start = 6;
    if(space == LCC_MEMORY_SPACE_CONFIGURATION_SPACE){
        response[1] = 0x19;
    }else if(space == LCC_MEMORY_SPACE_ALL_MEMORY){
        response[1] = 0x1A;
    }else if(space == LCC_MEMORY_SPACE_CONFIGURATION_DEFINITION){
        response[1] = 0x1B;
    }else{
        response[1] = 0x18;
        response[6] = space;
        start = 7;
    }

    lcc_uint32_to_data(response + 2, starting_address);
    response[start] = ((error_code & 0xFF00) >> 8);
    response[start + 1] = ((error_code & 0x00FF) >> 0);

    int total_len = start + 2;
    if(message){
        int msg_len = strlen(message);
        if(msg_len <= 62){
            memcpy(response + total_len, message, msg_len);
            total_len += msg_len;
            response[total_len] = 0;
        }
    }

    return lcc_datagram_load_and_send(ctx->parent->datagram_context,
                               alias,
                               response, total_len);

}

int lcc_memory_respond_read_reply_ok(struct lcc_memory_context* ctx,
                                     uint16_t alias,
                                   uint8_t space,
                                   uint32_t starting_address,
                                     void* data,
                                     int data_len){
    if(ctx == NULL || data_len < 0 || data_len > 64){
        return LCC_ERROR_INVALID_ARG;
    }

    uint8_t response[72];
    int start = 6;
    response[0] = 0x20;
    if(space == LCC_MEMORY_SPACE_CONFIGURATION_SPACE){
        response[1] = 0x51;
    }else if(space == LCC_MEMORY_SPACE_ALL_MEMORY){
        response[1] = 0x52;
    }else if(space == LCC_MEMORY_SPACE_CONFIGURATION_DEFINITION){
        response[1] = 0x53;
    }else{
        response[1] = 0x50;
        response[6] = space;
        start = 7;
    }

    lcc_uint32_to_data(response + 2, starting_address);

    if(space == LCC_MEMORY_SPACE_CONFIGURATION_DEFINITION){
        // Use our (somewhat inefficient) method for getting bytes from the CDI
        // so we can read from PROGMEM on Arduino
        for(int x = 0; x < data_len; x++ ){
            response[start + x] = lcc_memory_byte_from_data(data, x, ctx->cdi_flags);
        }
    }else{
        memcpy(response + start, data, data_len);
    }

    return lcc_datagram_load_and_send(ctx->parent->datagram_context,
                               alias,
                               response, start + data_len);
}

int lcc_memory_respond_read_reply_fail(struct lcc_memory_context* ctx,
                                       uint16_t alias,
                                   uint8_t space,
                                       uint32_t starting_address,
                                       uint16_t error_code,
                                       const char* message){
    uint8_t response[72];
    int total_len;
    int start = 6;
    response[0] = 0x20;
    if(space == LCC_MEMORY_SPACE_CONFIGURATION_SPACE){
        response[1] = 0x59;
    }else if(space == LCC_MEMORY_SPACE_ALL_MEMORY){
        response[1] = 0x5A;
    }else if(space == LCC_MEMORY_SPACE_CONFIGURATION_DEFINITION){
        response[1] = 0x5B;
    }else{
        response[1] = 0x58;
        response[6] = space;
        start = 7;
    }

    lcc_uint32_to_data(response + 2, starting_address);
    response[start] = ((error_code & 0xFF00) >> 8);
    response[start + 1] = ((error_code & 0x00FF) >> 0);

    total_len = start + 2;
    if(message){
        int msg_len = strlen(message);
        if(msg_len <= 62){
            memcpy(response + total_len, message, msg_len);
            total_len += msg_len;
            response[total_len] = 0;
        }
    }

    return lcc_datagram_load_and_send(ctx->parent->datagram_context,
                               alias,
                               response, total_len);
}

static int lcc_memory_handle_datagram_cdi_memory_space(struct lcc_memory_context* ctx, uint16_t alias, uint8_t* data, int data_len){
    lcc_datagram_respond_rxok(ctx->parent->datagram_context, alias, LCC_DATAGRAM_REPLY_PENDING);

    int resp = lcc_memory_respond_information_query(ctx,
                                                    alias,
                                                    1,
                                                    LCC_MEMORY_SPACE_CONFIGURATION_DEFINITION,
                                                    ctx->cdi_length,
                                                    0,
                                                    0);

    if(resp == 0){
        return 1;
    }

    return 0;
}

static int lcc_memory_handle_datagram_read_cdi_space(struct lcc_memory_context* ctx, uint16_t alias, uint8_t* data, int data_len){
    if(data_len < 6 || data_len > 7){
        return 0;
    }

    int space = 0;
    int addr_offset = 6;
    if(data[1] == 0x43){
        space = LCC_MEMORY_SPACE_CONFIGURATION_DEFINITION;
    }else if(data[1] == 0x40){
        space = data[6];
        addr_offset = 7;
    }

    if(space != LCC_MEMORY_SPACE_CONFIGURATION_DEFINITION){
        return 0;
    }

    uint8_t num_bytes_to_read = data[addr_offset];

    // At this point, we have determined that we have a valid read command for the CDI
    // that we can do something with.
    // First respond with the OK datagram.
    lcc_datagram_respond_rxok(ctx->parent->datagram_context, alias, LCC_DATAGRAM_REPLY_PENDING);

    // Let's go and read the data and return it.
    uint32_t starting_address = lcc_uint32_from_data(data + 2);

    int stat = lcc_memory_respond_read_reply_ok(ctx, alias, space, starting_address, ctx->cdi_data + starting_address, num_bytes_to_read);

    if(stat == 0){
        return 1;
    }else{
        return 0;
    }
}

int lcc_memory_try_handle_datagram(struct lcc_memory_context* ctx, uint16_t alias, uint8_t* data, int data_len){
    if(ctx->cdi_data == NULL){
        return 0;
    }

    if(data[0] != 0x20){
        return 0;
    }

    // Check to see if this is a 'get configuration options' command
    if(data[1] == 0x80){
        // TODO finish this option
    }

    if(data[1] == 0x84 &&
            data[2] == LCC_MEMORY_SPACE_CONFIGURATION_DEFINITION){
        return lcc_memory_handle_datagram_cdi_memory_space(ctx, alias, data, data_len);
    }else if(data[1] == 0x84){
        // return information for the given address space
        ctx->query_fn(ctx, alias, data[2]);
        return LCC_OK;
    }

    int handled = 0;
    handled = lcc_memory_handle_datagram_read_cdi_space(ctx, alias, data, data_len);
    if(handled == 1) return 1;

    // Determine if this is a read command or a write command and call the appropriate callback
    int space = 0;
    int is_read = 0;
    uint32_t starting_address = lcc_uint32_from_data(data + 2);
    int count = data[6];
    int data_offset = 6;
    if(data[1] == 0x43){
        space = LCC_MEMORY_SPACE_CONFIGURATION_DEFINITION;
        is_read = 1;
    }else if(data[1] == 0x42){
        space = LCC_MEMORY_SPACE_ALL_MEMORY;
        is_read = 1;
    }else if(data[1] == 0x41){
        space = LCC_MEMORY_SPACE_CONFIGURATION_SPACE;
        is_read = 1;
    }else if(data[1] == 0x40){
        space = data[6];
        is_read = 1;
        count = data[7];
        data_offset = 7;
    }else if(data[1] == 0x01){
        space = LCC_MEMORY_SPACE_CONFIGURATION_SPACE;
    }else if(data[1] == 0x02){
        space = LCC_MEMORY_SPACE_ALL_MEMORY;
    }else if(data[1] == 0x03){
        space = LCC_MEMORY_SPACE_CONFIGURATION_DEFINITION;
    }else if(data[1] == 0x00){
        space = data[6];
        data_offset = 7;
    }

    if(is_read && ctx->read_fn){
        // Send a 'datagram received ok' message with the 'reply pending' bit set,
        // indicating that we will have a follow-in message with the response of the read
        lcc_datagram_respond_rxok(ctx->parent->datagram_context, alias, LCC_DATAGRAM_REPLY_PENDING);

        ctx->read_fn(ctx, alias, space, starting_address, count);
    }else if(!is_read && ctx->write_fn){
        // We must first send a 'datagram received ok' message with the
        // REPLY_PENDING bit set in order to indicate that we will have a follow-on message
        // with the respones of the write.
        lcc_datagram_respond_rxok(ctx->parent->datagram_context, alias, LCC_DATAGRAM_REPLY_PENDING);

        ctx->write_fn(ctx, alias, space, starting_address, data + data_offset, data_len - data_offset);
    }else{
        // Unable to handle, send rejection back
        lcc_datagram_respond_rejected(ctx->parent->datagram_context, alias, 0x00, NULL);
    }

    return 1;
}
