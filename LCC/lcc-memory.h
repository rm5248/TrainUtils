/* SPDX-License-Identifier: GPL-2.0 */
#ifndef LIBLCC_MEMORY_H
#define LIBLCC_MEMORY_H

#include <stdint.h>

#include "lcc.h"

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
 * Read memory from the specified location.  Note that this will read a single block of
 * memory from the LCC device(max 64 bytes of data).  If you want to read an entire
 * memory section, this method must be called multiple times in order to transfer all of the data.
 *
 * @param ctx
 * @param alias
 * @param space
 * @param starting_address
 * @param read_count
 * @return
 */
int lcc_memory_read_single_transfer(struct lcc_context* ctx, int alias, uint8_t space, uint32_t starting_address, int read_count);

/**
 * Send a request to the specified node alias for memory space information.
 *
 * Note that the reply to this comes back as a datagram reply and must be parsed by higher level code.
 *
 * @param ctx
 * @param alias
 * @param space
 * @return
 */
int lcc_memory_get_address_space_information(struct lcc_context* ctx, int alias, uint8_t space);

#ifdef __cplusplus
} /* extern C */
#endif

#endif // LIBLCC_MEMORY_H
