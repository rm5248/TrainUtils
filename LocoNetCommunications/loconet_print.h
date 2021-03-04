/**
 * Contains function definitions for printing out various Loconet information
 */

#ifndef LOCONET_PRINT_H
#define LOCONET_PRINT_H

#include <sys/types.h>
#include <stdio.h>

#include "loconet_buffer.h"

#ifdef	__cplusplus
extern "C" {
#endif

/**
 * Given a byte of data, print out the direction of the locomotive,
 * plus the functions that are on( F0-F4 )
 */
void loconet_print_directions_and_func( FILE* output, uint8_t byte );

/**
 * Given a byte of data, print out the status.  this is the SLOT_STATUS1 byte.
 */
void loconet_print_slot_status( FILE* output, uint8_t stat );

/**
 * Print out the status of the track, as given by the trk byte
 */
void loconet_print_track_status( FILE* output, uint8_t trk );

/**
 * Print out the message in a text format
 */
void loconet_print_message( FILE* output, const struct loconet_message* message );

/**
 * Print out the message as hex bytes
 */
void loconet_print_message_hex( FILE* output, const struct loconet_message* message );

#ifdef	__cplusplus
}
#endif

#endif
