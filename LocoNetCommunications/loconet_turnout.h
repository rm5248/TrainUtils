#ifndef LOCONET_SWITCH_H
#define LOCONET_SWITCH_H

#ifdef	__cplusplus
extern "C" {
#endif

#define LOCONET_TURNOUT_FLAG_OUTPUT_ON_ONLY (0x01 << 0)

enum loconet_turnout_status {
    LOCONET_SWITCH_UNKNOWN,
    LOCONET_SWITCH_THROWN,
    LOCONET_SWITCH_CLOSED
};

struct loconet_turnout_manager;
struct loconet_message;
struct loconet_context;

typedef void (*loconet_turnout_changed_function)( struct loconet_turnout_manager*, int switch_num, enum loconet_turnout_status state );

struct loconet_turnout_manager* loconet_turnout_manager_new(struct loconet_context* parent);

void loconet_turnout_manager_free(struct loconet_turnout_manager*);

int loconet_turnout_manager_incoming_message(struct loconet_turnout_manager* manager, struct loconet_message* message);

/**
 * Throw the specified turnout(turnout numbers start at 1)
 *
 * @param manager
 * @param switch_num
 * @param flags Flags to use when throwing the switch.
 * @return
 */
int loconet_turnout_manager_throw(struct loconet_turnout_manager* manager, int switch_num, int flags);

/**
 * Close the switch(switch numbers start at 1)
 *
 * @param manager
 * @param switch_num
 * @param flags Flags to use when closing the switch
 * @return
 */
int loconet_turnout_manager_close(struct loconet_turnout_manager* manager, int switch_num, int flags);

int loconet_turnout_manager_set_turnout_state_changed_callback(struct loconet_turnout_manager* manager, loconet_turnout_changed_function fn);

/**
 * Get the state of the switch(switch numbers start at 1)
 *
 * @param manager
 * @param switch_num
 * @return
 */
enum loconet_turnout_status loconet_turnout_manager_get_cached_turnout_state(struct loconet_turnout_manager* manager, int switch_num);

void loconet_turnout_manager_set_userdata(struct loconet_turnout_manager* manager, void* user_data);

void* loconet_turnout_manager_userdata(struct loconet_turnout_manager* manager);

#ifdef	__cplusplus
} /* extern C */
#endif

#endif /* LOCONET_SWITCH_H */
