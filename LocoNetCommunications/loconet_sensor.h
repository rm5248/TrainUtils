#ifndef LOCONET_SENSOR_H
#define LOCONET_SENSOR_H

#ifdef	__cplusplus
extern "C" {
#endif

enum loconet_sensor_status {
    LOCONET_SENSOR_UNKNOWN,
    LOCONET_SENSOR_ACTIVE,
    LOCONET_SENSOR_INACTIVE
};

struct loconet_sensor_manager;
struct loconet_message;
struct loconet_context;

typedef void (*loconet_sensor_changed_function)( struct loconet_sensor_manager*, int sensor_num, enum loconet_sensor_status state );

struct loconet_sensor_manager* loconet_sensor_manager_new(struct loconet_context* parent);

void loconet_sensor_manager_free(struct loconet_sensor_manager*);

int loconet_sensor_manager_incoming_message(struct loconet_sensor_manager* manager, struct loconet_message* message);

int loconet_sensor_manager_set_sensor_state_changed_callback(struct loconet_sensor_manager* manager, loconet_sensor_changed_function fn);

enum loconet_sensor_status loconet_sensor_manager_get_cached_sensor_state(struct loconet_sensor_manager* manager, int sensor_num);

void loconet_sensor_manager_set_userdata(struct loconet_sensor_manager* manager, void* user_data);

void* loconet_sensor_manager_userdata(struct loconet_sensor_manager* manager);


#ifdef	__cplusplus
} /* extern C */
#endif

#endif /* LOCONET_SENSOR_H */
