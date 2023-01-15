/* SPDX-License-Identifier: GPL-2.0 */
#include "lcc-gridconnect.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

struct lcc_gridconnect{
    char buffer[128];
    int buffer_pos;
    struct lcc_can_frame frame;
    lcc_gridconnect_frame_parsed frame_parsed;
    void* user_data;
};

struct lcc_gridconnect* lcc_gridconnect_new(){
    struct lcc_gridconnect* gc = malloc(sizeof(struct lcc_gridconnect));
    memset(gc, 0, sizeof(struct lcc_gridconnect));

    return gc;
}

void lcc_gridconnect_free(struct lcc_gridconnect* gc){
    free(gc);
}

int lcc_gridconnect_incoming_data(struct lcc_gridconnect* context, void* data, uint32_t len){
    if(!context || !data) return LCC_ERROR_INVALID_ARG;

    uint8_t* u8_data = (uint8_t*)data;
    int discardUntilColon = context->buffer_pos == 0;
    for(uint32_t x = 0; x < len; x++){
        if(discardUntilColon && u8_data[x] != ':'){
            continue;
        }
        discardUntilColon = 0;
        context->buffer[context->buffer_pos] = u8_data[x];
        if(context->buffer[context->buffer_pos] == ';'){
            // Hey, look at that.  We should have a full packet
            context->buffer[context->buffer_pos + 1] = 0;
            if(lcc_gridconnect_to_canframe(context->buffer, &context->frame) == 0 &&
                    context->frame_parsed){
                context->frame_parsed(context, &context->frame);
            }
            context->buffer_pos = -1;
            discardUntilColon = 1;
        }
        context->buffer_pos++;
        if(context->buffer_pos >= sizeof(context->buffer)){
            context->buffer_pos = 0;
            discardUntilColon = 1;
        }
    }

    return LCC_OK;
}

int lcc_canframe_to_gridconnect(struct lcc_can_frame* frame, char* output, int out_len){
    if(frame == NULL || output == NULL) return LCC_ERROR_INVALID_ARG;
    char data_bytes[17]; // data is 16 chars at most(plus one for null terminator)
    snprintf(data_bytes, sizeof(data_bytes), "%02X%02X%02X%02X%02X%02X%02X%02X",
             frame->data[0],
            frame->data[1],
            frame->data[2],
            frame->data[3],
            frame->data[4],
            frame->data[5],
            frame->data[6],
            frame->data[7]);
    // Put the null at the correct spot in the string
    int null_spot = 17;
    if(frame->can_len < 8){
        null_spot = (2 * frame->can_len);
    }

    data_bytes[null_spot] = 0;

    snprintf(output, out_len, ":X%XN%s;", frame->can_id, data_bytes);

    return LCC_OK;
}

int lcc_gridconnect_to_canframe(char* ascii, struct lcc_can_frame* frame){
    if(ascii == NULL || frame == NULL) return LCC_ERROR_INVALID_ARG;

    char str_buffer[12];
    int str_buffer_pos = 0;
    int num_bytes = 0;

    memset(frame, 0, sizeof(struct lcc_can_frame));

    int len = strlen(ascii);
    if(ascii[0] != ':' ||
            ascii[len - 1] != ';'){
        return LCC_ERROR_INVALID_ARG;
    }

    // Walk the string, convert data as appropriate
    int x = 1;
    if(ascii[x] == 'S'){
        // Standard frame
    }else if(ascii[x] == 'X'){
        // Extended frame
    }

    x++;
    while(ascii[x] != 'N'){
        str_buffer[str_buffer_pos] = ascii[x];
        str_buffer_pos++;
        x++;
    }
    str_buffer[str_buffer_pos + 1] = 0;
    str_buffer_pos = 0;
    frame->can_id = strtol(str_buffer, NULL, 16);

    // For each two bytes that we get next, set the appropriate data byte in our frame
    x++;
    str_buffer[2] = 0;
    while(ascii[x] != ';'){
        str_buffer[0] = ascii[x];
        str_buffer[1] = ascii[x + 1];
        x+=2;
        frame->data[num_bytes] = strtol(str_buffer, NULL, 16);
        num_bytes++;
    }

    frame->can_len = num_bytes;

    return LCC_OK;
}

int lcc_gridconnect_set_frame_parsed(struct lcc_gridconnect* context, lcc_gridconnect_frame_parsed frame_parsed_fn){
    if(context == NULL) return LCC_ERROR_INVALID_ARG;

    context->frame_parsed = frame_parsed_fn;

    return LCC_OK;
}

void lcc_gridconnect_set_userdata(struct lcc_gridconnect* ctx, void* user_data){
    if(!ctx) return;
    ctx->user_data = user_data;
}

void* lcc_gridconnect_user_data(struct lcc_gridconnect* ctx){
    return ctx->user_data;
}
