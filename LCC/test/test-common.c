/* SPDX-License-Identifier: GPL-2.0 */
#include <stdlib.h>
#include "test-common.h"
#include "lcc.h"

static struct lcc_context** all_ctx = NULL;
static int num_ctx = 0;

static int write_fun(struct lcc_context* ctx, struct lcc_can_frame* frame){
    struct lcc_can_frame new_frame = *frame;

    for(int x = 0; x < num_ctx; x++){
        if(ctx == all_ctx[x]){
            continue;
        }

        lcc_context_incoming_frame(all_ctx[x], &new_frame);
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

        lcc_context_set_write_function(all_ctx[x], write_fun);
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

    return all_ctx;
}

void lcctest_free_contexts(){
    for(int x = 0; x < num_ctx; x++){
        lcc_context_free(all_ctx[x]);
    }
}
