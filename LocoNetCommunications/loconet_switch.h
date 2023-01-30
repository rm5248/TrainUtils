#ifndef LOCONET_SWITCH_H
#define LOCONET_SWITCH_H

#ifdef	__cplusplus
extern "C" {
#endif

enum loconet_switch_state_status {
    LOCONET_SWITCH_UNKNOWN,
    LOCONET_SWITCH_THROWN,
    LOCONET_SWITCH_CLOSED
};

struct loconet_switch_manager;
struct loconet_message;
struct loconet_context;

typedef void (*loconet_switch_changed_function)( struct loconet_switch_manager*, int switch_num, enum loconet_switch_state_status state );

struct loconet_switch_manager* loconet_switch_manager_new(struct loconet_context* parent);

void loconet_switch_manager_free(struct loconet_switch_manager*);

int loconet_switch_manager_incoming_message(struct loconet_switch_manager* manager, struct loconet_message* message);

/**
 * Throw the specified switch(switch numbers start at 1)
 *
 * @param manager
 * @param switch_num
 * @return
 */
int loconet_switch_manager_throw_switch(struct loconet_switch_manager* manager, int switch_num);

/**
 * Close the switch(switch numbers start at 1)
 *
 * @param manager
 * @param switch_num
 * @return
 */
int loconet_switch_manager_close_switch(struct loconet_switch_manager* manager, int switch_num);

int loconet_switch_manager_set_switch_state_changed_callback(struct loconet_switch_manager* manager, loconet_switch_changed_function fn);

/**
 * Get the state of the switch(switch numbers start at 1)
 *
 * @param manager
 * @param switch_num
 * @return
 */
enum loconet_switch_state_status loconet_switch_manager_get_cached_switch_state(struct loconet_switch_manager* manager, int switch_num);

void loconet_switch_manager_set_userdata(struct loconet_switch_manager* manager, void* user_data);

void* loconet_switch_manager_userdata(struct loconet_switch_manager* manager);

#ifdef	__cplusplus
} /* extern C */
#endif

#endif /* LOCONET_SWITCH_H */
