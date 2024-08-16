/* SPDX-License-Identifier: GPL-2.0 */
#ifndef LCCFIRMWAREUPGRADE_H
#define LCCFIRMWAREUPGRADE_H

#include "lcc-common.h"

struct lcc_firmware_upgrade_context;
struct lcc_context;

#define LCC_FIRMWARE_UPGRADE_ERROR_TRANSFER 0x2088 /* Temporary error, firmwar written has failed checksum */
#define LCC_FIRMWARE_UPGRADE_ERROR_INVALID_ARGS 0x1088 /* Permanent error, invalid arguments.  The firmware is incompatible with this hardware node */
#define LCC_FIRMWARE_UPGRADE_ERROR_IVALID 0x1089 /* Permanent error, invalid arguments.  The firmware is invalid or corrupted */

#ifdef __cplusplus
extern "C" {
#endif

struct lcc_firmware_upgrade_context* lcc_firmware_upgrade_new(struct lcc_context* ctx);

/**
 * Check to see if a firmware upgrade context is in progress.
 *
 * @param ctx
 * @return 1 if a firmware upgrade is in progress, 0 if not, <0 on error
 */
int lcc_firmware_upgrade_in_progress(struct lcc_firmware_upgrade_context* ctx);

/**
 * Set the callback functions that are required for the firmware upgrade to work.
 *
 * @param ctx
 * @param start_fn A function that will be called when the firwmare upgrade starts.
 * @param incoming_fn A function that will be called when a block of firmware upgrade comes in
 * @param finished_fn A function that will be called once the CT sends the "Unfreeze" command to this node.
 * The firmware should be properly transferred at this point, but it is possible that it has not been properly
 * sent - you must validate the firmware before attempting to run the new version.
 * @return
 */
int lcc_firmware_upgrade_set_functions(struct lcc_firmware_upgrade_context* ctx,
                                       lcc_firmware_upgrade_start start_fn,
                                       lcc_firmware_upgrade_incoming_data incoming_fn,
                                       lcc_firmware_upgrade_finished finished_fn);

/**
 * This function must be called from the lcc_firmware_upgrade_incoming_data callback in order to indicate that the data
 * was written OK.
 *
 * @param ctx
 * @return
 */
int lcc_firmware_write_ok(struct lcc_firmware_upgrade_context* ctx);

/**
 * This function must be called from the lcc_firmware_upgrade_incoming_data callback in order to indicate that the data
 * was unable to be written.  Nodes may use the error code defines that start with LCC_FIRMWARE_UPGRADE_ERROR_XXX.
 *
 * @param ctx
 * @return
 */
int lcc_firmware_write_error(struct lcc_firmware_upgrade_context* ctx,
                             uint16_t error_code,
                             const char* optional_info);

#ifdef __cplusplus
} /* extern C */
#endif

#endif // LCCFIRMWAREUPGRADE_H
