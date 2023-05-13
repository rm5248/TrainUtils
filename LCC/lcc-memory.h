/* SPDX-License-Identifier: GPL-2.0 */
#ifndef LIBLCC_MEMORY_H
#define LIBLCC_MEMORY_H

#ifdef __cplusplus
extern "C" {
#endif

struct lcc_memory;
struct lcc_context;

//typedef void(*lcc_memory_read_cb)

struct lcc_memory* lcc_memory_new(struct lcc_context* ctx);

void lcc_memory_free(struct lcc_memory* memory);

//int lcc_memory_set_read_callback(struct lcc_memory* memory, )

/**
 * Read memory from the specified location
 *
 * @param memory
 * @param alias
 * @param space
 * @param starting_address
 * @param read_count
 * @return
 */
int lcc_memory_read(struct lcc_memory* memory, int alias, int space, int starting_address, int read_count);

#ifdef __cplusplus
} /* extern C */
#endif

#endif // LIBLCC_MEMORY_H
