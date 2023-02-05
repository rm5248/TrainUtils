#include <stdint.h>
#include <stdlib.h>

#include "loconet_sensor.h"
#include "loconet_buffer.h"

struct loconet_sensor_manager{
    loconet_sensor_changed_function sensor_changed_cb;
    struct loconet_context* parent;
    void* user_data;
    // 2 bits are needed per sensor, which means we can have 4 sensors per byte
    // There are a total of 2048 possible sensors on Loconet, so that gives us
    // 512 bytes of switch status
    uint8_t cached_sensor_status[512];
};

struct loconet_sensor_manager* loconet_sensor_manager_new(struct loconet_context* parent){
    struct loconet_sensor_manager* manager = calloc(1, sizeof(struct loconet_sensor_manager));
    manager->parent = parent;

    return manager;
}

void loconet_sensor_manager_free(struct loconet_sensor_manager* manager){
    if(manager){
        free(manager);
    }
}

int loconet_sensor_manager_incoming_message(struct loconet_sensor_manager* manager, struct loconet_message* message){
    if(message->opcode != LN_OPC_INPUT_REPORT){
        return LN_OK;
    }

    struct loconet_inputs inputs = message->inputs;
    int sensor_num = ((inputs.in2 & 0x0F) << 7) | inputs.in1 ;
    int status_offset = sensor_num / 4;
    int sensor_offset = sensor_num % 4;
    enum loconet_sensor_status new_status;
    enum loconet_sensor_status old_status;
    int old_status_int = (manager->cached_sensor_status[status_offset] & ~(0x3 << sensor_offset)) >> sensor_offset;
    if(old_status_int == 1){
        old_status = LOCONET_SENSOR_ACTIVE;
    }else if(old_status_int == 2){
        old_status = LOCONET_SENSOR_INACTIVE;
    }else{
        old_status = LOCONET_SENSOR_UNKNOWN;
    }
    if(inputs.in2 & (0x01 << 5)){
        new_status = LOCONET_SENSOR_ACTIVE;
        manager->cached_sensor_status[status_offset] &= ~(0x3 << sensor_offset);
        manager->cached_sensor_status[status_offset] |= (0x01 << sensor_offset);
    }else{
        new_status = LOCONET_SENSOR_INACTIVE;
        manager->cached_sensor_status[status_offset] &= ~(0x3 << sensor_offset);
        manager->cached_sensor_status[status_offset] |= (0x02 << sensor_offset);
    }

    if(manager->sensor_changed_cb &&
            (new_status != old_status)){
        manager->sensor_changed_cb(manager, sensor_num + 1, new_status);
    }

    return LN_OK;
}

int loconet_sensor_manager_set_sensor_state_changed_callback(struct loconet_sensor_manager* manager, loconet_sensor_changed_function fn){
    if(!manager) return LN_ERROR_INVALID_ARG;
    manager->sensor_changed_cb = fn;
    return LN_OK;
}

enum loconet_sensor_status loconet_sensor_manager_get_cached_sensor_state(struct loconet_sensor_manager* manager, int sensor_num){
    if(sensor_num <= 1 || sensor_num > 2048){
        return LN_ERROR_INVALID_SENSOR_NUM;
    }
    sensor_num -= 1;
    int status_offset = sensor_num / 4;
    int switch_offset = sensor_num % 4;
    int status = manager->cached_sensor_status[status_offset] & (0x3 << sensor_num);
    status = status >> switch_offset;

    if(status == 0x01){
        return LOCONET_SENSOR_ACTIVE;
    }else if(status == 0x02){
        return LOCONET_SENSOR_INACTIVE;
    }

    return LOCONET_SENSOR_UNKNOWN;
}

void loconet_sensor_manager_set_userdata(struct loconet_sensor_manager* manager, void* user_data){
    if(!manager) return;
    manager->user_data = user_data;
}

void* loconet_sensor_manager_userdata(struct loconet_sensor_manager* manager){
    if(manager == NULL) return NULL;
    return manager->user_data;
}
