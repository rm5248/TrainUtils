/* SPDX-License-Identifier: GPL-2.0 */
#ifndef LIBLCC_MEMORY_H
#define LIBLCC_MEMORY_H

#include <stdint.h>

#include "lcc.h"

#define LCC_MEMORY_SPACE_CONFIGURATION_DEFINITION 0xFF
#define LCC_MEMORY_SPACE_ALL_MEMORY 0xFE
#define LCC_MEMORY_SPACE_CONFIGURATION_SPACE 0xFD

#define LCC_MEMORY_CDI_FLAG_NONE    0
//#define LCC_MEMORY_CDI_FLAG_COMPRESSED (0x01 << 0)
#define LCC_MEMORY_CDI_FLAG_ARDUINO_PROGMEM (0x01 << 1)

#ifdef __cplusplus
extern "C" {
#endif

struct lcc_memory_context;
struct lcc_context;

//typedef void(*lcc_memory_read_cb)

struct lcc_memory_context* lcc_memory_new(struct lcc_context* ctx);

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

/**
 * Set pointer to CDI data for this node.  If the CDI data is set, then queries
 * for the CDI will automatically be handled by the library.
 *
 * If you need special handling for your CDI, do not set this data.
 *
 * @param ctx
 * @param cdi_data the raw CDI data
 * @param cdi_len The length of the CDI data.
 * @param flags Flags for the data.
 * @return
 */
int lcc_memory_set_cdi(struct lcc_memory_context* ctx, void* cdi_data, int cdi_len, int flags);

int lcc_memory_set_memory_functions(struct lcc_memory_context* ctx,
                                    lcc_address_space_information_query query_fn,
                                    lcc_address_space_read read_fn,
                                    lcc_address_space_write write_fn);

int lcc_memory_respond_information_query(struct lcc_memory_context* ctx,
                                         uint16_t alias,
                                          uint8_t address_space_present,
                                          uint8_t address_space,
                                          uint32_t highest_address,
                                          uint8_t flags,
                                          uint32_t lowest_address);

int lcc_memory_respond_write_reply_ok(struct lcc_memory_context* ctx,
                                      uint16_t alias,
                                   uint8_t space,
                                   uint32_t starting_address);

int lcc_memory_respond_write_reply_fail(struct lcc_memory_context* ctx,
                                        uint16_t alias,
                                   uint8_t space,
                                   uint32_t starting_address,
                                        uint16_t error_code,
                                        const char* message);

int lcc_memory_respond_read_reply_ok(struct lcc_memory_context* ctx,
                                     uint16_t alias,
                                   uint8_t space,
                                   uint32_t starting_address,
                                     void* data,
                                     int data_len);

int lcc_memory_respond_read_reply_fail(struct lcc_memory_context* ctx,
                                       uint16_t alias,
                                   uint8_t space,
                                       uint32_t starting_address,
                                   uint16_t error_code,
                                       const char* message);

#ifdef __cplusplus
} /* extern C */
#endif

#endif // LIBLCC_MEMORY_H
