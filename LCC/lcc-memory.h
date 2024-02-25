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

#define LCC_MEMORY_FLAG_SPACE_READ_ONLY (0x01)
#define LCC_MEMORY_FLAG_SPACE_WRITEABLE (0x00)

#ifdef __cplusplus
extern "C" {
#endif

struct lcc_memory_context;
struct lcc_context;

/**
 * Create a new memory context.  The memory context is for sending data from your node.
 * If you want to query a remote node, use a lcc_remote_memory_context object.
 * These are split to save memory.  On a normal LCC node, you do not care about reading
 * memory from a remote node, you are only interested in providing your memory to other
 * node(such as a configuration tool like JMRI).
 *
 * @param ctx
 * @return
 */
struct lcc_memory_context* lcc_memory_new(struct lcc_context* ctx);

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

/**
 * Set a function that will be called when a reboot is requested.
 *
 * @param ctx
 * @param reboot_fn
 * @return
 */
int lcc_memory_set_reboot_function(struct lcc_memory_context* ctx, lcc_reboot reboot_fn);

/**
 * Set a function that will be called when a factory reset is requested.
 *
 * @param ctx
 * @param reset_fn
 * @return
 */
int lcc_memory_set_factory_reset_function(struct lcc_memory_context* ctx, lcc_factory_reset reset_fn);

/**
 * Set functions for reading/writing/querying memory on the device.
 *
 * @param ctx
 * @param query_fn Function to be called when a remote device is querying us for memory information
 * ("Get Address Space Information Command" - Memory Configuration, section 4.13)
 * @param read_fn Function to be called when a remote device is trying to read memory
 * @param write_fn Function to be called when a remote device is trying to write memory
 * @return LCC_OK if successful, else error code
 */
int lcc_memory_set_memory_functions(struct lcc_memory_context* ctx,
                                    lcc_address_space_information_query query_fn,
                                    lcc_address_space_read read_fn,
                                    lcc_address_space_write write_fn);

/**
 * Respond to the given device with memory space information.  This should be called from within the
 * lcc_address_space_information_query callback function, as set by the
 * lcc_memory_set_functions function.
 *
 * This corresponds to "Get Address Space Information Reply" in the LCC specification(Memory Configuration, section 4.16)
 *
 * @param ctx
 * @param alias The alias that queried the information.  This is passed in from the callback function
 * @param address_space_present Set to 1 if the address space is present, 0 if not
 * @param address_space The space that is being queried.  Passed in from the callback function
 * @param highest_address The highest address that this space has
 * @param flags Should be either LCC_MEMORY_FLAG_SPACE_READ_ONLY or LCC_MEMORY_FLAG_SPACE_WRITEABLE
 * @param lowest_address The lowest address that the memory space has.  Generally this is 0.
 * @return LCC error code
 */
int lcc_memory_respond_information_query(struct lcc_memory_context* ctx,
                                         uint16_t alias,
                                          uint8_t address_space_present,
                                          uint8_t address_space,
                                          uint32_t highest_address,
                                          uint8_t flags,
                                          uint32_t lowest_address);

/**
 * Respond to the calling device that the write was OK.
 * This should be called from within the lcc_address_space_write function callback as set from
 * lcc_memory_set_memory_functions.
 *
 * @param ctx
 * @param alias The alias that wrote to us(passed in from callback)
 * @param space The space we were writing to
 * @param starting_address The starting address we were writing to.
 * @return
 */
int lcc_memory_respond_write_reply_ok(struct lcc_memory_context* ctx,
                                      uint16_t alias,
                                   uint8_t space,
                                   uint32_t starting_address);

/**
 * Respond to the calling device that the write failed.
 * This should be called from within the lcc_address_space_write function callback as set from
 * lcc_memory_set_memory_functions.
 *
 * @param ctx
 * @param alias The alias that wrote to us(passed in from callback)
 * @param space The space we were writing to
 * @param starting_address The starting address we were writing to.
 * @param error_code The error code
 * @param message Optional message describing the error.  May be NULL.
 * @return
 */
int lcc_memory_respond_write_reply_fail(struct lcc_memory_context* ctx,
                                        uint16_t alias,
                                   uint8_t space,
                                   uint32_t starting_address,
                                        uint16_t error_code,
                                        const char* message);

/**
 * Respond to the calling device that the read was OK.
 * This should be called from within the lcc_address_space_read function callback as set from
 * lcc_memory_set_memory_functions.
 *
 * LibLCC copies 'data' as needed.
 *
 * @param ctx
 * @param alias The alias that read to us(passed in from callback)
 * @param space The space that we were reading from
 * @param starting_address The starting address we were reading from
 * @param data The data that was read
 * @param data_len How long the data is
 * @return
 */
int lcc_memory_respond_read_reply_ok(struct lcc_memory_context* ctx,
                                     uint16_t alias,
                                   uint8_t space,
                                   uint32_t starting_address,
                                     void* data,
                                     int data_len);

/**
 * Respond to the calling device that the read failed.
 * This should be called from within the lcc_address_space_read function callback as set from
 * lcc_memory_set_memory_functions.
 *
 * @param ctx
 * @param alias The alias that read from us(passed in from callback)
 * @param space The space that we were reading from
 * @param starting_address The starting address we were reading from
 * @param error_code The error code
 * @param message Optional message describing the error.  May be NULL.
 * @return
 */
int lcc_memory_respond_read_reply_fail(struct lcc_memory_context* ctx,
                                       uint16_t alias,
                                   uint8_t space,
                                       uint32_t starting_address,
                                   uint16_t error_code,
                                       const char* message);

struct lcc_context* lcc_memory_parent_context(struct lcc_memory_context* ctx);

#ifdef __cplusplus
} /* extern C */
#endif

#endif // LIBLCC_MEMORY_H
