/* SPDX-License-Identifier: GPL-2.0 */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "lcc.h"
#include "lcc-common-internal.h"
#include "lcc-common.h"
#include "lcc-simple.h"
#include "lcc-addressed.h"

static void lcc_send_amd_frame(struct lcc_context* ctx){
    struct lcc_can_frame amd_frame;
    amd_frame.can_len = 0;
    amd_frame.res0 = 0;
    amd_frame.res1 = 0;
    amd_frame.res2 = 0;

    // AMD frame
    lcc_set_lcb_variable_field(&amd_frame, ctx, 0x701);
    amd_frame.can_id &= (~(0x01l << 27));
    lcc_set_nodeid_in_data(&amd_frame, ctx->unique_id);
    ctx->write_function(ctx, &amd_frame);
}

static void lcc_handle_control_frame(struct lcc_context* ctx, struct lcc_can_frame* frame){
    uint16_t frame_type = (frame->can_id & LCC_VARIABLE_FIELD_MASK) >> 12;

    if(frame_type == 0x702){
        // Alias Mapping Enquiry(AME) frame
        uint64_t node_id = 0;
        if(frame->can_len == 6){
            node_id = lcc_get_node_id_from_data(frame);
        }

        if(ctx->state != LCC_STATE_PERMITTED){
            return;
        }

        // If the full node ID is equal to our ID and we are in the permitted state,
        // respond with AMD frame
        if(ctx->state == LCC_STATE_PERMITTED){
            lcc_send_amd_frame(ctx);
        }

        // If there is no data content, also respond with an AMD frame
        if(frame->can_len == 0){
            lcc_send_amd_frame(ctx);
        }
    }else if(frame_type == 0x703){
        // Alias Map Reset(AMR) frame

    }
}

static void lcc_context_check_collision(struct lcc_context* ctx, struct lcc_can_frame* frame){
    int is_frame_control = 1;
    uint16_t node_alias = frame->can_id & 0xFFF;
    int cid_frame_number = (frame->can_id >> 24ll) & 0x07;
    int is_cid_frame;


    is_cid_frame = is_frame_control && cid_frame_number <= 0x07 && cid_frame_number >= 0x04;

    if(frame->can_id & (0x01l << 27) ){
        // bit 27: 1 for LCC message, 0 for CAN control frame
        is_frame_control = 0;
    }

    if(node_alias != ctx->node_alias){
        // No collision, break out early
        return;
    }

    // A node shall compare the source Node ID alias in each received frame against
    // all reserved Node ID
    // aliases it currently holds. In case of a match, the receiving node shall:

    // If the frame is a CID frame, send an RID frame in response
    if(is_cid_frame){
        struct lcc_can_frame tosend;
        tosend.can_len = 0;
        tosend.res0 = 0;
        tosend.res1 = 0;
        tosend.res2 = 0;

        // RID frame
        lcc_set_lcb_variable_field(&tosend, ctx, 0x700);
        tosend.can_id &= (~(0x01l << 27));
        ctx->write_function(ctx, &tosend);
        return;
    }

    // If the frame is not a Check ID (CID) frame, the node is in Permitted state,
    // and the received
    // source Node ID alias is the current Node ID alias of the node, the node shall
    // immediately
    // transition to Inhibited state, send an AMR frame to release and then stop
    // using the current Node
    // ID alias.
    // TODO put code here

    // If the frame is not a Check ID (CID) frame and the node is not in Permitted
    // state, the node shall
    // immediately stop using the matching Node ID alias.
    if( !is_cid_frame && ctx->state != LCC_STATE_PERMITTED ){
        ctx->node_alias_state = LCC_NODE_ALIAS_FAIL;
    }

    // If the frame is not a Check ID (CID) frame and the received source
    // Node ID alias is not the
    // current Node ID alias of the node, the node shall immediately
    // stop using the matching node ID
    // alias.
}

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

    lcc_context_check_collision(ctx, frame);

    if(ctx->state != LCC_STATE_PERMITTED){
        return LCC_OK;
    }

    // Decode the CAN frame and maybe do something useful with it.
    if((frame->can_id & LCC_FRAME_TYPE_MASK) == 0ll){
        lcc_handle_control_frame(ctx, frame);
        return LCC_OK;
    }

    uint16_t mti = (frame->can_id & LCC_VARIABLE_FIELD_MASK) >> 12;
    uint8_t frame_type = (frame->can_id & LCC_CAN_FRAME_TYPE_MASK) >> 24;
    uint16_t source_alias = (frame->can_id & LCC_NID_ALIAS_MASK);
    int can_frame_type = (frame->can_id & LCC_CAN_FRAME_TYPE_MASK) >> 24;

    // TODO the handlers below should probably be in a list of some kind,
    // so that we just call them sequentially until somebody can handle the request
    if(is_datagram_frame(frame)){
        LOG_DEBUG("lcc.context", "Going to try to handle datagram" );
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

int lcc_context_set_write_function(struct lcc_context* ctx, lcc_write_fn write_fn, lcc_write_buffer_available write_buffer_avail_fn){
    if( !write_fn || !ctx ){
        return LCC_ERROR_INVALID_ARG;
    }

    ctx->write_function = write_fn;
    ctx->write_buffer_avail_function = write_buffer_avail_fn;

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
        // Simple LCG: https://en.wikipedia.org/wiki/Linear_congruential_generator
        // https://rosettacode.org/wiki/Linear_congruential_generator#C
        uint16_t alias = 0;
        alias = ctx->unique_id & 0xFFF;
        uint32_t new_val = (alias * 1103515245llu + 12345);
        alias = new_val & 0xFFF;

        ctx->node_alias = alias;
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

    int write_ret = LCC_OK;
    // Send our four CID frames
    struct lcc_can_frame frame;
    frame.can_len = 0;
    frame.res0 = 0;
    frame.res1 = 0;
    frame.res2 = 0;

    lcc_set_lcb_variable_field(&frame, ctx, ((ctx->unique_id & 0xFFF000000000llu) >> 36) | 0x7000);
    // this is a CAN control frame, clear bit 27
    frame.can_id &= (~(0x01l << 27));
    write_ret = ctx->write_function(ctx, &frame);
    if(write_ret != LCC_OK){
        goto write_err;
    }

    lcc_set_lcb_variable_field(&frame, ctx, ((ctx->unique_id & 0x000FFF000000llu) >> 24) | 0x6000);
    frame.can_id &= (~(0x01l << 27));
    write_ret = ctx->write_function(ctx, &frame);
    if(write_ret != LCC_OK){
        goto write_err;
    }

    lcc_set_lcb_variable_field(&frame, ctx, ((ctx->unique_id & 0x000000FFF000llu) >> 12) | 0x5000);
    frame.can_id &= (~(0x01l << 27));
    write_ret = ctx->write_function(ctx, &frame);
    if(write_ret != LCC_OK){
        goto write_err;
    }

    lcc_set_lcb_variable_field(&frame, ctx, ((ctx->unique_id & 0x000000000FFFllu) >> 0) | 0x4000);
    frame.can_id &= (~(0x01l << 27));
    write_ret = ctx->write_function(ctx, &frame);
    if(write_ret != LCC_OK){
        goto write_err;
    }

    ctx->node_alias_state = LCC_NODE_ALIAS_SENT_CID;

write_err:
    return write_ret;
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

#ifdef LCC_SIMPLE_NODE_INFO_SMALL
static void lcc_context_small_info_string_copy(struct lcc_context* ctx, int idx, const char* field, int field_max_len){
    int field_len = 0;
    if(field){
        field_len = strlen(field);
    }
    int bytes_to_copy = field_len;
    if(field_len > field_max_len){
        bytes_to_copy = field_max_len - 1;
    }

    if(idx == 0){
        if(field){
            memcpy(ctx->simple_info.node_information, field, bytes_to_copy);
            ctx->simple_info.node_information[bytes_to_copy + 1] = 0;
        }else{
            ctx->simple_info.node_information[0] = 0;
        }
        return;
    }

    // First thing to do: find where to put this
    char* string_start_location = NULL;
    int current_field = 0;
    for(unsigned int x = 1; x < sizeof(ctx->simple_info); x++){
        if(ctx->simple_info.node_information[x - 1] == 0){
            current_field++;
            if(current_field == idx){
                string_start_location = ctx->simple_info.node_information + x;
                break;
            }
        }
    }

    if(!string_start_location){
        // can't find where to put this.  So we don't put it in.
        return;
    }

    // Copy the string, making sure that we NULL terminate it and we don't run off the end
    unsigned int current_location;
    for(current_location = 0; current_location < bytes_to_copy; current_location++){
        char* next_location = string_start_location + current_location + 1;
        if(next_location >= ctx->simple_info.node_information + sizeof(ctx->simple_info.node_information)){
            break;
        }

        string_start_location[current_location] = field[current_location];
    }
    string_start_location[current_location + 1] = 0;
}
#endif

int lcc_context_set_simple_node_information(struct lcc_context* ctx,
                                            const char* manufacturer_name,
                                            const char* model_name,
                                            const char* hw_version,
                                            const char* sw_version){
    if( !ctx ){
        return LCC_ERROR_INVALID_ARG;
    }

    int manufacturer_string_len = manufacturer_name ? strlen(manufacturer_name) : 0;
    int model_string_len = model_name ? strlen(model_name) : 0;
    int hardware_string_len = hw_version ? strlen(hw_version) : 0;
    int software_string_len = sw_version ? strlen(sw_version) : 0;

    if(manufacturer_string_len > 40 ||
            model_string_len > 40 ||
            hardware_string_len > 20 ||
            software_string_len > 20){
        return LCC_ERROR_STRING_TOO_LONG;
    }

#ifdef LCC_SIMPLE_NODE_INFO_SMALL
    // Copy all of the strings back-to-back
    lcc_context_small_info_string_copy(ctx, 0, manufacturer_name, 40);
    lcc_context_small_info_string_copy(ctx, 1, model_name, 40);
    lcc_context_small_info_string_copy(ctx, 2, hw_version, 20);
    lcc_context_small_info_string_copy(ctx, 3, sw_version, 20);
#else
    memset(ctx->simple_info.manufacturer_name, 0, 124);
    if(manufacturer_name){
        memcpy(ctx->simple_info.manufacturer_name, manufacturer_name, manufacturer_string_len + 1);
    }
    if(model_name){
        memcpy(ctx->simple_info.model_name, model_name, model_string_len + 1);
    }
    if(hw_version){
        memcpy(ctx->simple_info.hw_version, hw_version, hardware_string_len + 1);
    }
    if(sw_version){
        memcpy(ctx->simple_info.sw_version, sw_version, software_string_len + 1);
    }
#endif

    return LCC_OK;
}

int lcc_context_set_simple_node_name_description(struct lcc_context* ctx,
                                                 const char* node_name,
                                                 const char* node_description){
    if( !ctx ){
        return LCC_ERROR_INVALID_ARG;
    }

    int node_name_len = node_name ? strlen(node_name) : 0;
    int node_description_len = node_description ? strlen(node_description) : 0;

    if(node_name_len > 62 ||
            node_description_len > 63){
        return LCC_ERROR_STRING_TOO_LONG;
    }

#ifdef LCC_SIMPLE_NODE_INFO_SMALL
    lcc_context_small_info_string_copy(ctx, 4, node_name, 62);
    lcc_context_small_info_string_copy(ctx, 5, node_description, 63);
#else
    memcpy(ctx->simple_info.node_name, node_name, node_name_len + 1);
    memcpy(ctx->simple_info.node_description, node_description, node_description_len + 1);
#endif

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

struct lcc_remote_memory_context* lcc_context_get_remote_memory_context(struct lcc_context* ctx){
    if(!ctx){
        return NULL;
    }

    return ctx->remote_memory_context;
}

uint32_t lcc_library_version(){
    // On Arduino, we can't get the library version from the cmake configuration file.
    // We can at least check the versions when using CMake.
#define MAJOR 0ll
#define MINOR 5ll
#define MICRO 0ll

#ifdef LIBLCC_MAJOR

#if LIBLCC_MAJOR != MAJOR
#error "Major number bad!"
#endif

#if LIBLCC_MINOR != MINOR
#error "Minor number bad!"
#endif

#if LIBLCC_MICRO != MICRO
#error "Micro number bad!"
#endif

#endif

    const uint32_t lib_version = MAJOR << 16 |
                                          MINOR << 8 |
                                          MICRO << 0;
    return lib_version;
}

void lcc_set_log_function(simplelogger_log_function log_fn){
    lcc_global_log = log_fn;
}

