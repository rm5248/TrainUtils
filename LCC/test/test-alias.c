/* SPDX-License-Identifier: GPL-2.0 */
#include <string.h>
#include <stdio.h>

#include "lcc.h"
#include "lcc-common.h"

struct lcc_context *both_ctx[2];

static int tx_buffer_fn(struct lcc_context* ctx){
    return 1;
}

static int write_fun(struct lcc_context* ctx, struct lcc_can_frame* frame){
    struct lcc_can_frame new_frame = *frame;
    if(ctx == both_ctx[0]){
        lcc_context_incoming_frame(both_ctx[1], &new_frame);
    }else{
        lcc_context_incoming_frame(both_ctx[0], &new_frame);
    }

    return LCC_OK;
}

static int alias_collision(){
    // Create two nodes that will generate the same alias,
    // ensure that they will both get differing aliases
    int stat;

    both_ctx[0] = lcc_context_new();
    both_ctx[1] = lcc_context_new();

    stat = lcc_context_set_unique_identifer( both_ctx[0], 0x55 );
    if(stat != LCC_OK){
        fprintf(stderr, "bad\n");
        return 1;
    }
    stat = lcc_context_set_unique_identifer( both_ctx[1], 0x55 );
    if(stat != LCC_OK){
        fprintf(stderr, "bad\n");
        return 1;
    }

    lcc_context_set_write_function( both_ctx[0], write_fun, NULL );
    lcc_context_set_write_function( both_ctx[1], write_fun, NULL );

    stat = lcc_context_generate_alias( both_ctx[0] );
    if(stat != LCC_OK){
        fprintf(stderr, "can't generate alias\n");
        return 1;
    }
    stat = lcc_context_generate_alias( both_ctx[1] );
    if(stat != LCC_OK){
        fprintf(stderr, "can't generate alias\n");
        return 1;
    }

    stat = lcc_context_claim_alias( both_ctx[0] );
    if(stat != LCC_OK){
        fprintf(stderr, "ctx 0 can't claim: %d\n", stat);
        return 1;
    }

    stat = lcc_context_claim_alias( both_ctx[1] );
    if(stat == LCC_OK){
        fprintf(stderr, "ctx 1 claimed alias\n");
        return 1;
    }

    // ctx[1] should have to generate a new alias
    lcc_context_generate_alias( both_ctx[1] );
    stat = lcc_context_claim_alias( both_ctx[1] );
    if(stat != LCC_OK){
        fprintf(stderr, "ctx 1 can't claim alias\n");
        return 1;
    }

    if(lcc_context_alias(both_ctx[0]) == lcc_context_alias(both_ctx[1])){
        fprintf(stderr, "both ctx same alias\n");
        return 1;
    }

    printf("alias: 0x%04X : 0x%04X\n",
            lcc_context_alias(both_ctx[0]),
            lcc_context_alias(both_ctx[1]));

    return 0;
}

/**
 * Check to make sure that if we try to claim the alias when our tx buffer
 * is not empty, an error is returned.
 */
static int alias_buffer_not_tx(){
    int stat;

    both_ctx[0] = lcc_context_new();

    stat = lcc_context_set_unique_identifer( both_ctx[0], 0x55 );
    if(stat != LCC_OK){
        fprintf(stderr, "bad\n");
        return 1;
    }

    lcc_context_set_write_function( both_ctx[0], write_fun, tx_buffer_fn );

    stat = lcc_context_generate_alias( both_ctx[0] );
    if(stat != LCC_OK){
        fprintf(stderr, "can't generate alias\n");
        return 1;
    }

    stat = lcc_context_claim_alias( both_ctx[0] );
    if(stat != LCC_ERROR_ALIAS_TX_NOT_EMPTY){
        return 1;
    }

    return 0;
}

int main(int argc, char** argv){
    if(argc < 2) return 1;

    if(strcmp(argv[1], "alias-collision") == 0){
        return alias_collision();
    }else if(strcmp(argv[1], "alias-buffer-not-tx") == 0){
        return alias_buffer_not_tx();
    }

    return 1;
}
