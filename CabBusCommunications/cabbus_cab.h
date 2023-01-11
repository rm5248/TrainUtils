#ifndef CABBUS_CAB_H
#define CABBUS_CAB_H

#ifdef	__cplusplus
extern "C" {
#endif

#include <stdint.h>

enum cabbus_direction {
    CAB_DIR_FORWARD,
    CAB_DIR_REVERSE
};

/**
 * Opaque data structure that represents a Cab on the cab bus.
 */
struct cabbus_cab;

/**
 * Set the cab locomotive number
 * @param number
 */
void cabbus_cab_set_loco_number( struct cabbus_cab*, int number );

/**
 * Set the cab speed.
 */
void cabbus_cab_set_loco_speed( struct cabbus_cab*, uint8_t speed );

/**
 * Set the time displayed on the cab
 * @param
 * @param hour
 * @param minute
 * @param am
 */
void cabbus_cab_set_time( struct cabbus_cab*, char hour, char minute, char am );

/**
 * Set a function of the cab, it being either on or off.
 */
void cabbus_cab_set_functions( struct cabbus_cab*, char functionNum, char on );

/**
 * Set the direction on this cab.
 */
void cabbus_cab_set_direction( struct cabbus_cab*, enum cabbus_direction direction );

/**
 * Get the current locomotive number of this cab
 */
uint16_t cabbus_cab_get_loco_number( struct cabbus_cab* );

/**
 * Get the latest command from this cab
 */
struct cab_command* cabbus_cab_get_command( struct cabbus_cab* );

/**
 * Ask a yes/no question to the user.
 */
void cabbus_cab_ask_question( struct cabbus_cab*, const char* );

/**
 * Get the network number of this cab
 */
uint8_t cabbus_cab_get_cab_number( struct cabbus_cab* );

/**
 * Give a message to the user
 */
void cabbus_cab_user_message( struct cabbus_cab*, const char* );

/**
 * Set user data for this cab
 */
void cabbus_cab_set_user_data( struct cabbus_cab*, void* );

/**
 * Get user data for this cab
 *
 * @param
 * @return
 */
void* cabbus_cab_get_user_data( struct cabbus_cab* );

/**
 * Get if the specified function is on or off.
 *
 * @param
 * @param function The function number to check
 * @return TRUE or FALSE depending on if the function is active.
 */
int cabbus_cab_get_function( struct cabbus_cab*, uint8_t function );

/**
 * Get the current speed of the cab.
 *
 * @param cab
 * @return
 */
int cabbus_cab_get_speed( struct cabbus_cab* cab );

#ifdef	__cplusplus
}
#endif

#endif /* CABBUS_CAB_H */
