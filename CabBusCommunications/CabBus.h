/*
* File:   CabBus.h
* Author: Rob
*
* Created on November 29, 2013, 3:58 PM
*/

#ifndef CABBUS_H
#define	CABBUS_H

#include <inttypes.h>

#include "cab_commands.h"
#include "cabbus_cab.h"

#ifdef	__cplusplus
extern "C" {
#endif

/**
 * This function is called to delay a given number of milliseconds.
 *
 * @param delayMs The number of milliseconds to delay.
 */
typedef void (*cab_delay_fn)( uint32_t delayMs );

/**
 * This function is called to actually write data out to the bus.
 *
 * @param data The data to write out
 * @param len The length of the data to write out(in bytes)
 */
typedef void (*cab_write_fn)( void* data, uint8_t len );

/**
 * This function is called to see if there is currently data coming
 * in before we try to ping the next address
 *
 * @return 1 if there is data incoming, 0 otherwise
 */
typedef uint32_t (*cab_incoming_data)(void);


// Forward declaration of private cab type
struct cabbus_context;

/**
 * Called to initialize all the cabs.
 *
 * @param delay Function pointer to a function that will
 * delay for the specified number of ms
 * @param write Function pointer to a function that will
 * write data out to the cabbus.
 */
struct cabbus_context* cabbus_new( cab_delay_fn delay,
                                  cab_write_fn write);

void cabbus_free( struct cabbus_context* ctx );

/**
 * Send a ping to the next cab.  Step 1 of the ping process.
 */
void cabbus_ping_step1( struct cabbus_context* ctx );

/**
 * Return the cab, if we were able to ping it.
 * Any data from the serial port must have been put into our buffer with
 * cabbus_incoming_byte before calling this function.
 */
struct cabbus_cab* cabbus_ping_step2( struct cabbus_context* ctx );

struct cabbus_cab* cabbus_cab_by_id( struct cabbus_context* ctx, int id );

/**
 * Call this when a byte comes in on the bus
 */
void cabbus_incoming_byte( struct cabbus_context* ctx, uint8_t byte );

void cabbus_set_user_data( struct cabbus_context* ctx, void* user_data );

void* cabbus_get_user_data( struct cabbus_context* ctx );

/**
 * Set the time on all connected cabs at once.
 *
 * @param ctx
 * @param hours
 * @param minutes
 * @param amPm
 */
void cabbus_set_all_cab_times( struct cabbus_context* ctx, int hours, int minutes, _Bool am );

#ifdef	__cplusplus
}
#endif

#endif	/* CABBUS_H */

