#ifndef LOCONET_THROTTLE_H
#define LOCONET_THROTTLE_H

#ifdef	__cplusplus
extern "C" {
#endif

#define LOCONET_THROTTLE_SELECT_FLAG_SHORT_ADDR (0x01 << 0)
#define LOCONET_THROTTLE_SELECT_FLAG_LONG_ADDR  0
#define LOCONET_THROTTLE_SELECT_FLAG_AUTO_STEAL (0x01 << 1)

/**
 * A loconet_throttle lets you control a single locomotive
 */
struct loconet_throttle;
struct loconet_context;
struct loconet_message;

/**
 * A callback that will be called when selecting a locomotive, telling us that the locomotive is already in use
 * and seeing if we want to steal.
 */
typedef void (*loconet_throttle_prompt_steal)(struct loconet_throttle* throttle);

/**
 * A callback that will be called once we have acquired control of a locomotive.
 */
typedef void (*loconet_throttle_locomotive_controlled)(struct loconet_throttle* throttle);

struct loconet_throttle* loconet_throttle_new(struct loconet_context* ctx);

void loconet_throttle_free(struct loconet_throttle* throttle);

/**
 * When a message comes in over loconet, notify the throttle of new data
 *
 * @param msg
 */
void loconet_throttle_incoming_message(struct loconet_throttle* throttle, struct loconet_message* msg);

/**
 * Set the speed of the locomotive.  This does not handle EStop.
 *
 * @param throttle
 * @param speed 0-127
 * @return
 */
int loconet_throttle_set_speed(struct loconet_throttle* throttle, int speed);

/**
 * ESTOP the currently controlled locomotive
 *
 * @param throttle
 * @return
 */
int loconet_throttle_estop(struct loconet_throttle* throttle);

/**
 * Configure the function to be on or off.
 *
 * @param throttle
 * @param function
 * @param on
 * @return
 */
int loconet_throttle_set_function(struct loconet_throttle* throttle, int function, int on);

/**
 * Attempt to select a locomotive.
 *
 * @param throttle
 * @param locomotive_number
 * @param select_flags Flags that control how to select a locomotive
 * @return
 */
int loconet_throttle_select_locomotive(struct loconet_throttle* throttle, int locomotive_number, int select_flags);

int loconet_throttle_set_prompt_steal_callback(struct loconet_throttle* throttle, loconet_throttle_prompt_steal cb);

int loconet_throttle_set_locomotive_controlled_callback(struct loconet_throttle* throttle, loconet_throttle_locomotive_controlled cb );

/**
 * When asking if we want to steal the locomotive, call this function to confirm.
 *
 * @param throttle
 * @return
 */
int loconet_throttle_confirm_steal(struct loconet_throttle* throttle);

int loconet_throttle_abort_loco_selection(struct loconet_throttle* throttle);

/**
 * Update the slot in the command staion by pinging it(send current speed value so loco does not get purged).
 * Pings should happen about every 100 seconds if there is no other activity.
 *
 * @param throttle
 * @return
 */
int loconet_throttle_slot_update(struct loconet_throttle* throttle);

/**
 * Dispatch(release) the slot used by this throttle.
 *
 * @param throttle
 * @return
 */
int loconet_throttle_dispatch(struct loconet_throttle* throttle);

int loconet_throttle_get_locomotive_number(struct loconet_throttle* throttle);

/**
 * Set the direction.
 *
 * @param throttle
 * @param direction 1 = forward, 0 = reverse
 * @return
 */
int loconet_throttle_set_direction(struct loconet_throttle* throttle, int direction);

/**
 * Get the state of the given function on the throttle.  The state will be 0(for off) or 1(for on),
 * or a negative error value.
 *
 * @param throttle
 * @param function
 * @return
 */
int loconet_throttle_get_function_state(struct loconet_throttle* throttle, int function);

int loconet_throttle_set_userdata(struct loconet_throttle* throttle, void* user_data);

void* loconet_throttle_userdata(struct loconet_throttle* throttle);

#ifdef	__cplusplus
}
#endif

#endif /* LOCONET_THROTTLE_H */
