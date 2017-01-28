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
    typedef uint32_t (*cab_incoming_data)();


    // Forward declaration of private cab type
    struct Cab;

    enum Direction {
        FORWARD,
        REVERSE
    };

    /**
     * Called to initialize all the cabs.
     */
    void cabbus_init( cab_delay_fn inDelay, cab_write_fn inWrite, cab_incoming_data incoming );

    /**
     * Ping the next cab on the bus
     *
     * @return The cab, if some data needs to be sent out
     */
    struct Cab* cabbus_ping_next();

    /**
     * Set the cab locomotive number
     * @param number
     */
    void cabbus_set_loco_number( struct Cab*, int number );

    /**
     * Set the cab speed.
     */
    void cabbus_set_loco_speed( struct Cab*, uint8_t speed );

    /**
     * Set the time displayed on the cab
     * @param
     * @param hour
     * @param minute
     * @param am
     */
    void cabbus_set_time( struct Cab*, char hour, char minute, char am );

    /**
     * Set a function of the cab, it being either on or off.
     */
    void cabbus_set_functions( struct Cab*, char functionNum, char on );

    /**
     * Set the direction on this cab.
     */
    void cabbus_set_direction( struct Cab*, enum Direction direction );

    /**
     * Call this when a byte comes in on the bus
     */
    void cabbus_incoming_byte( uint8_t byte );

    /**
     * Get the current locomotive number of this cab
     */
    uint16_t cabbus_get_loco_number( struct Cab* );

    /**
     * Get the latest command from this cab
     */
    struct cab_command* cabbus_get_command( struct Cab* );

    /**
     * Ask a yes/no question to the user.  
     */
    void cabbus_ask_question( struct Cab*, const char* );

    /**
     * Get the network number of this cab
     */
    uint8_t cabbus_get_cab_number( struct Cab* );

    /**
     * Give a message to the user
     */
    void cabbus_user_message( struct Cab*, const char* );

    /**
     * Set user data for this cab
     */
    void cabbus_set_user_data( struct Cab*, void* );

    /**
     * Get user data for this cab
     * 
     * @param
     * @return
     */
    void* cabbus_get_user_data( struct Cab* );

    /**
     * Get if the specified function is on or off.
     * 
     * @param
     * @param function The function number to check
     * @return TRUE or FALSE depending on if the function is active.
     */
    int cabbus_get_function( struct Cab*, uint8_t function );
    

#ifdef	__cplusplus
}
#endif

#endif	/* CABBUS_H */

