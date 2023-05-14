/* SPDX-License-Identifier: GPL-2.0 */
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "lcc-memory.h"
#include "lcc-common-internal.h"

struct lcc_memory{
    struct lcc_context* ctx;
};

//static void lcc_memory_datagram_finished(struct lcc_context* ctx, void* datagram_data, int len){

//}

struct lcc_memory* lcc_memory_new(struct lcc_context* ctx){
    struct lcc_memory* memory = malloc(sizeof(struct lcc_memory));

    memset(memory, 0, sizeof(struct lcc_memory));
    memory->ctx = ctx;

    return memory;
}

void lcc_memory_free(struct lcc_memory* memory){
    free(memory);
}

int lcc_memory_read_single_transfer(struct lcc_context* ctx, int alias, uint8_t space, uint32_t starting_address, int read_count){
    memset(&ctx->incoming_datagram, 0, sizeof(ctx->incoming_datagram));

    if(read_count > 64 || read_count < 0){
        return LCC_ERROR_INVALID_ARG;
    }

    struct lcc_can_frame frame;
    memset(&frame, 0, sizeof(frame));

    lcc_set_lcb_variable_field(&frame, ctx, alias);
    lcc_set_lcb_can_frame_type(&frame, 1);

    // The frame format is 0x1Adddsss
    int real_id = frame.can_id & 0xFFFFFF;
    real_id |= 0x1A000000;
    frame.can_id = real_id;

    frame.can_len = 7;
    frame.data[0] = 0x20;
    frame.data[1] = (0x01 << 6); /* read operation */
    if(space == LCC_MEMORY_SPACE_CONFIGURATION_DEFINITION){
        frame.data[1] |= 0x3;
    }else if(space == LCC_MEMORY_SPACE_ALL_MEMORY){
        frame.data[1] |= 0x2;
    }else if(space == LCC_MEMORY_SPACE_CONFIGURATION_SPACE){
        frame.data[1] |= 0x01;
    }else{
        frame.data[6] = space;
    }
    frame.data[2] = ((starting_address & 0xFF000000) >> 24) & 0xFF;
    frame.data[3] = ((starting_address & 0x00FF0000) >> 16) & 0xFF;
    frame.data[4] = ((starting_address & 0x0000FF00) >> 8) & 0xFF;
    frame.data[5] = ((starting_address & 0x000000FF) >> 0) & 0xFF;
    frame.data[6] = read_count & 0xFF;
    ctx->write_function(ctx, &frame);

    return LCC_OK;
}
