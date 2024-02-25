/* SPDX-License-Identifier: GPL-2.0 */
#include <stdlib.h>
#include "test-common.h"
#include "lcc.h"

static struct lcc_context** all_ctx = NULL;
static int num_ctx = 0;

// Linked list that contains the frames we need to send.
// This is so we can pop at the front
struct can_frame_meta{
    struct lcc_context* from_ctx;
    struct lcc_can_frame frame;
    struct can_frame_meta* next;
};

static struct can_frame_meta* frames_to_send = NULL;

static int write_fun(struct lcc_context* ctx, struct lcc_can_frame* frame){
    struct can_frame_meta* new_meta = malloc(sizeof(struct can_frame_meta));
    new_meta->from_ctx = ctx;
    new_meta->frame = *frame;
    new_meta->next = NULL;

    if(frames_to_send == NULL){
        frames_to_send = new_meta;
    }else{
        struct can_frame_meta* current = frames_to_send;
        while(current->next != NULL){
            current = current->next;
        }
        current->next = new_meta;
    }

    return LCC_OK;
}

struct lcc_context** lcctest_create_contexts(int num){
    if(all_ctx){
        // for test code we should not get here
        abort();
    }

    all_ctx = calloc( num, sizeof(struct lcc_context*) );
    num_ctx = num;
    uint64_t unique_id = 0x00000011;

    for(int x = 0; x < num; x++){
        all_ctx[x] = lcc_context_new();

        lcc_context_set_write_function(all_ctx[x], write_fun, NULL);
        lcc_context_set_unique_identifer(all_ctx[x], unique_id++);
    }

    // Claim alias on all contexts
    for(int x = 0; x < num; x++){
        int stat = lcc_context_generate_alias(all_ctx[x]);
        if(stat != LCC_OK){
            abort();
        }
    }
    for(int x = 0; x < num; x++){
        int stat = lcc_context_claim_alias(all_ctx[x]);
        if(stat != LCC_OK){
            // Collision? try again once
            stat = lcc_context_generate_alias(all_ctx[x]);
            stat = lcc_context_claim_alias(all_ctx[x]);
            if(stat != LCC_OK){
                abort();
            }
        }
    }

    // Pump the frames to have the network setup
    lcctest_pump_frames();

    return all_ctx;
}

void lcctest_free_contexts(){
    for(int x = 0; x < num_ctx; x++){
        lcc_context_free(all_ctx[x]);
    }
}

void lcctest_pump_frames(){
    if(frames_to_send == NULL){
        return;
    }

    while(frames_to_send != NULL){
        struct can_frame_meta* current_frame = frames_to_send;

        frames_to_send = current_frame->next;

        for(int x = 0; x < num_ctx; x++){
            struct lcc_context* curr_ctx = all_ctx[x];
            if(curr_ctx == current_frame->from_ctx){
                continue;
            }

            lcc_context_incoming_frame(curr_ctx, &current_frame->frame);
        }

        free(current_frame);
    }
}
