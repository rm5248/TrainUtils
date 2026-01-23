/* SPDX-License-Identifier: GPL-2.0 */
#include <stdlib.h>
#include <string.h>

#include "lcc-train-control.h"
#include "lcc-common-internal.h"

struct lcc_train_control_context{
    struct lcc_context* parent;
    void* user_data;
    lcc_time_query_fn query_time_fn;
};

struct lcc_train_control_context* lcc_train_control_context_new(struct lcc_context* ctx){
    if(ctx->train_control == NULL){
        return NULL;
    }

    if(ctx->train_control){
        return ctx->train_control;
    }

#ifdef LIBLCC_ENABLE_STATIC_CONTEXT
    static struct lcc_train_control_context train_ctl;
    memset(&train_ctl, 0, sizeof(train_ctl));
    train_ctl.parent = ctx;
    ctx->train_control = &train_ctl;

    return &train_ctl;
#else
    struct lcc_train_control_context* train_ctl = malloc(sizeof(struct lcc_train_control_context));

    memset(train_ctl, 0, sizeof(struct lcc_train_control_context));
    train_ctl->parent = ctx;
    ctx->train_control = train_ctl;

    return train_ctl;
#endif
}

int lcc_train_control_set_user_data(struct lcc_train_control_context* ctx, void* user_data){
    if(ctx == NULL){
        return LCC_ERROR_INVALID_ARG;
    }

    ctx->user_data = user_data;
    return LCC_OK;
}

void* lcc_train_control_user_data(struct lcc_train_control_context* ctx){
    if(ctx == NULL){
        return NULL;
    }
    return ctx->user_data;
}

int lcc_train_control_set_time_query_function(struct lcc_train_control_context* ctx, lcc_time_query_fn query_fn){
    if(ctx == NULL){
        return LCC_ERROR_INVALID_ARG;
    }
    ctx->query_time_fn = query_fn;
    return LCC_OK;
}

int lcc_train_control_find_train_by_address(struct lcc_train_control_context* ctx,
                                             uint16_t address,
                                            enum LCC_Track_Protocol protocol_hints){
    if(ctx == NULL){
        return LCC_ERROR_INVALID_ARG;
    }

    struct lcc_can_frame frame = {0};
    uint64_t event_id = 0x090099FF00000000llu;

    // send 'identify producer' message with the event id
    lcc_set_lcb_variable_field(&frame, ctx->parent, LCC_MTI_EVENT_IDENTIFY_PRODUCER);
    lcc_set_lcb_can_frame_type(&frame, 1);
    lcc_set_eventid_in_data(&frame, event_id);
}

int lcc_train_control_allocate_train_node_by_address(struct lcc_train_control_context* ctx,
                                             uint16_t address,
                                             enum LCC_Track_Protocol protocol_hints,
                                                     lcc_train_node_allocated allocated_cb){
    if(ctx == NULL){
        return LCC_ERROR_INVALID_ARG;
    }
}

// --------------------------------------
// LCC Train Node functions
// --------------------------------------
int lcc_train_node_free(struct lcc_train_node* node){

}

int lcc_train_node_set_speed(struct lcc_train_node* train_node, uint8_t speed){

}

int lcc_train_node_set_direction(struct lcc_train_node* train_node, enum LCC_Train_Direction direction){

}

int lcc_train_node_set_function(struct lcc_train_node* train_node, uint8_t function, uint8_t value){

}
