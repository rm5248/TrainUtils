/* SPDX-License-Identifier: GPL-2.0 */
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "lcc-memory-map.h"
#include "lcc-memory.h"
#include "lcc-common-internal.h"

struct lcc_memory_map {
    struct lcc_memory_context* memory;
    struct lcc_memory_segment* segments;
    lcc_memory_map_written written_cb;
};

static struct lcc_memory_segment* find_segment(struct lcc_memory_map* map, uint8_t space){
    struct lcc_memory_segment* seg = map->segments;
    while(seg && seg->space != 0){
        if(seg->space == space){
            return seg;
        }
        seg++;
    }
    return NULL;
}

static void memory_map_query(struct lcc_memory_context* ctx, uint16_t alias, uint8_t address_space){
    struct lcc_memory_map* map = ctx->user_data;
    struct lcc_memory_segment* seg = find_segment(map, address_space);
    if(seg == NULL){
        lcc_memory_respond_information_query(ctx, alias, 0, address_space, 0, 0, 0);
        return;
    }

    lcc_memory_respond_information_query(ctx,
                                         alias,
                                         1,
                                         address_space,
                                         seg->high_address,
                                         seg->space_flags,
                                         seg->low_address);
}

static void memory_map_read(struct lcc_memory_context* ctx, uint16_t alias, uint8_t address_space, uint32_t starting_address, uint8_t read_count){
    struct lcc_memory_map* map = ctx->user_data;
    struct lcc_memory_segment* seg = find_segment(map, address_space);
    if(seg == NULL){
        lcc_memory_respond_read_reply_fail(ctx, alias, address_space, starting_address, LCC_ERRCODE_NOT_IMPLEMENTED, NULL);
        return;
    }

    if(starting_address < seg->low_address ||
       starting_address + read_count > seg->high_address + 1){
        lcc_memory_respond_read_reply_fail(ctx, alias, address_space, starting_address, LCC_ERRCODE_INVALID_ARG, NULL);
        return;
    }

    uint8_t* mem = (uint8_t*)seg->memory;
    lcc_memory_respond_read_reply_ok(ctx, alias, address_space, starting_address,
                                     mem + (starting_address - seg->low_address), read_count);
}

static void memory_map_write(struct lcc_memory_context* ctx, uint16_t alias, uint8_t address_space, uint32_t starting_address, void* data, int data_len){
    struct lcc_memory_map* map = ctx->user_data;
    struct lcc_memory_segment* seg = find_segment(map, address_space);
    if(seg == NULL){
        lcc_memory_respond_write_reply_fail(ctx, alias, address_space, starting_address, LCC_ERRCODE_NOT_IMPLEMENTED, NULL);
        return;
    }

    if(seg->space_flags & LCC_MEMORY_FLAG_SPACE_READ_ONLY){
        lcc_memory_respond_write_reply_fail(ctx, alias, address_space, starting_address, LCC_ERRCODE_INVALID_ARG, NULL);
        return;
    }

    if(starting_address < seg->low_address ||
       (uint32_t)(starting_address + data_len) > seg->high_address + 1){
        lcc_memory_respond_write_reply_fail(ctx, alias, address_space, starting_address, LCC_ERRCODE_INVALID_ARG, NULL);
        return;
    }

    uint8_t* mem = (uint8_t*)seg->memory;
    memcpy(mem + (starting_address - seg->low_address), data, data_len);
    lcc_memory_respond_write_reply_ok(ctx, alias, address_space, starting_address);

    if(map->written_cb){
        map->written_cb(map, address_space);
    }
}

struct lcc_memory_map* lcc_memory_map_new(struct lcc_memory_context* memory, const struct lcc_memory_segment* segments){
    if(memory == NULL || segments == NULL){
        return NULL;
    }

#ifdef LIBLCC_ENABLE_STATIC_CONTEXT
    static struct lcc_memory_map map;
    memset(&map, 0, sizeof(map));
    map.memory = memory;
    map.segments = segments;
    memory->user_data = &map;

    lcc_memory_set_memory_functions(memory, memory_map_query, memory_map_read, memory_map_write);

    return &map;
#else
    struct lcc_memory_map* map = malloc(sizeof(struct lcc_memory_map));

    memset(map, 0, sizeof(struct lcc_memory_map));
    map->memory = memory;
    map->segments = segments;
    memory->user_data = map;

    lcc_memory_set_memory_functions(memory, memory_map_query, memory_map_read, memory_map_write);

    return map;
#endif
}

int lcc_memory_map_free(struct lcc_memory_map* map){
    if(!map){
        return LCC_ERROR_INVALID_ARG;
    }

    lcc_memory_set_memory_functions(map->memory, NULL, NULL, NULL);

#ifdef LIBLCC_ENABLE_STATIC_CONTEXT
    memset(map, 0, sizeof(struct lcc_memory_map));
#else
    free(map);
#endif

    return 0;
}

int lcc_memory_map_set_written_callback(struct lcc_memory_map* map, lcc_memory_map_written segment_written_cb){
    if(map == NULL){
        return LCC_ERROR_INVALID_ARG;
    }

    map->written_cb = segment_written_cb;

    return LCC_OK;
}
