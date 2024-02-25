/* SPDX-License-Identifier: GPL-2.0 */
#ifndef LIBLCC_REMOTE_MEMORY_H
#define LIBLCC_REMOTE_MEMORY_H

#include <stdint.h>

#include "lcc-common.h"

#ifdef __cplusplus
extern "C" {
#endif

struct lcc_remote_memory_context;
struct lcc_memory_context;
struct lcc_context;

/**
 * Create a remote memory context.  The remote memory context is used for retreiving
 * memory data from a node on the bus.
 *
 * @param ctx
 * @return
 */
struct lcc_remote_memory_context* lcc_remote_memory_new(struct lcc_context* ctx);

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
int lcc_remote_memory_read_single_transfer(struct lcc_remote_memory_context* ctx, int alias, uint8_t space, uint32_t starting_address, int read_count);

/**
 * Send a request to the specified node alias for memory space information.
 *
 * @param ctx
 * @param alias The node to get the memory information for
 * @param space The space to get the memory information for
 * @return
 */
int lcc_remote_memory_get_address_space_information(struct lcc_remote_memory_context* ctx, int alias, uint8_t space);

/**
 * Set callback functions that are used to get memory back from a remote device.
 *
 * @param ctx
 * @param remote_ok Called when the receiving node responds with 'Datagram Received OK'
 * @param remote_fail Called if the remote node rejects the request
 * @param received_memory Called when the memory request is finished
 * @param read_rejected Called when the memory read is rejected from the remote node
 * @return
 */
int lcc_remote_memory_set_functions(struct lcc_remote_memory_context* ctx,
                                    lcc_remote_memory_request_ok remote_ok,
                                    lcc_remote_memory_request_fail remote_fail,
                                    lcc_remote_memory_received received_memory,
                                    lcc_remote_memory_read_rejected read_rejected );

/**
 * Get the alias of the node we are currently requesting memory from.  If we are not
 * currently querying anything from any node, returns 0.
 *
 * @param ctx
 * @return
 */
uint16_t lcc_remote_memory_current_alias_request(struct lcc_remote_memory_context* ctx);

struct lcc_context* lcc_remote_memory_parent(struct lcc_remote_memory_context* ctx);

#ifdef __cplusplus
} /* extern C */
#endif

#endif /* LIBLCC_REMOTE_MEMORY_H */
