/* SPDX-License-Identifier: GPL-2.0 */
#include <stdlib.h>
#include <string.h>

#include "lcc-memory.h"

struct lcc_memory{
    struct lcc_context* ctx;
};

struct lcc_memory* lcc_memory_new(struct lcc_context* ctx){
    struct lcc_memory* memory = malloc(sizeof(struct lcc_memory));

    memset(memory, 0, sizeof(struct lcc_memory));
    memory->ctx = ctx;

    return memory;
}

void lcc_memory_free(struct lcc_memory* memory){
    free(memory);
}

int lcc_memory_read(struct lcc_memory* memory, int alias, int space, int starting_address, int read_count){

}
