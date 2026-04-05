/* SPDX-License-Identifier: GPL-2.0 */
#ifndef LIBLCC_MEMORY_MAP_H
#define LIBLCC_MEMORY_MAP_H

#include <stdint.h>

/**
 * Provides a simple way of defining segments in memory.
 *
 * space - the space that this map entry corresponds to
 * space_flags - readonly or write
 * low_address - the lowest address in this memory segment
 * high_address - the highest address in this memory segment
 * memory - the base memory location
 */
struct lcc_memory_segment{
    uint8_t space;
    uint8_t space_flags;
    uint32_t low_address;
    uint32_t high_address;
    void* memory;
};

struct lcc_memory_map;
struct lcc_memory_context;

/**
 * Callback function called when a segment has been written to.
 */
typedef void (*lcc_memory_map_written)(struct lcc_memory_map* map, uint8_t space);

/**
 * Create a new memory map.  Note that this will install callback functions for the memory
 * context, so you may not use the low level lcc_memory_* functions when using a memory map.
 *
 * @param memory
 * @param segments The segments in this memory.  NULL terminated(last space is 0).
 * @return
 */
struct lcc_memory_map* lcc_memory_map_new(struct lcc_memory_context* memory, const struct lcc_memory_segment* segments);

int lcc_memory_map_free(struct lcc_memory_map* map);

/**
 * Set a callback function to be called when a segment in the map is written to.
 *
 * @param map
 * @param segment_written_cb
 * @return
 */
int lcc_memory_map_set_written_callback(struct lcc_memory_map* map, lcc_memory_map_written segment_written_cb);

#endif // LIBLCC_MEMORY_MAP_H
