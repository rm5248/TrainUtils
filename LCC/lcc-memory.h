/* SPDX-License-Identifier: GPL-2.0 */
#ifndef LIBLCC_MEMORY_H
#define LIBLCC_MEMORY_H

#include <stdint.h>

#define LCC_MEMORY_SPACE_CONFIGURATION_DEFINITION 0xFF
#define LCC_MEMORY_SPACE_ALL_MEMORY 0xFE
#define LCC_MEMORY_SPACE_CONFIGURATION_SPACE 0xFD

#ifdef __cplusplus
extern "C" {
#endif

struct lcc_memory;
struct lcc_context;

//typedef void(*lcc_memory_read_cb)

//struct lcc_memory* lcc_memory_new(struct lcc_context* ctx);

//void lcc_memory_free(struct lcc_memory* memory);

//int lcc_memory_set_read_callback(struct lcc_memory* memory, )

/**
 * Read memory from the specified location
 *
 * @param ctx
 * @param alias
 * @param space
 * @param starting_address
 * @param read_count
 * @return
 */
int lcc_memory_read_single_transfer(struct lcc_context* ctx, int alias, uint8_t space, uint32_t starting_address, int read_count);

#ifdef __cplusplus
} /* extern C */
#endif

#endif // LIBLCC_MEMORY_H
