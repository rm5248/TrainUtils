#include <stdlib.h>
#include "loconet_turnout.h"
#include "loconet_buffer.h"

struct loconet_turnout_manager{
    loconet_turnout_changed_function switch_changed_callback;
    struct loconet_context* parent;
    void* user_data;
    // 2 bits are needed per switch, which means we can have 4 switches per byte
    // There are a total of 2048 possible switches(DCC standard), so that gives us
    // 512 bytes of switch status
    uint8_t cached_switch_status[512];
};

struct loconet_turnout_manager* loconet_turnout_manager_new(struct loconet_context* parent){
    struct loconet_turnout_manager* manager = calloc(1, sizeof(struct loconet_turnout_manager));
    manager->parent = parent;

    return manager;
}

void loconet_turnout_manager_free(struct loconet_turnout_manager* manager){
    if(manager){
        free(manager);
    }
}

int loconet_turnout_manager_incoming_message(struct loconet_turnout_manager* manager, struct loconet_message* message){
    if(message->opcode != LN_OPC_SWITCH_REQUEST){
        return LN_OK;
    }

    struct loconet_request_switch req = message->req_switch;
    int switch_num = ((req.sw2 & 0x0F) << 7) | req.sw1 ;
    int status_offset = switch_num / 4;
    int switch_offset = switch_num % 4;
    enum loconet_turnout_status new_status;
    enum loconet_turnout_status old_status;
    int old_status_int = (manager->cached_switch_status[status_offset] & ~(0x3 << switch_offset)) >> switch_offset;
    if(old_status_int == 1){
        old_status = LOCONET_SWITCH_CLOSED;
    }else if(old_status_int == 2){
        old_status = LOCONET_SWITCH_THROWN;
    }else{
        old_status = LOCONET_SWITCH_UNKNOWN;
    }
    if(req.sw2 & (0x01 << 5)){
        new_status = LOCONET_SWITCH_CLOSED;
        manager->cached_switch_status[status_offset] &= ~(0x3 << switch_offset);
        manager->cached_switch_status[status_offset] |= (0x01 << switch_offset);
    }else{
        new_status = LOCONET_SWITCH_THROWN;
        manager->cached_switch_status[status_offset] &= ~(0x3 << switch_offset);
        manager->cached_switch_status[status_offset] |= (0x02 << switch_offset);
    }

    if(manager->switch_changed_callback &&
            (new_status != old_status)){
        manager->switch_changed_callback(manager, switch_num + 1, new_status);
    }

    return LN_OK;
}

int loconet_turnout_manager_throw(struct loconet_turnout_manager* manager, int switch_num, int flags){
    struct loconet_message msg;

    if(switch_num > 2048 || switch_num < 1){
        return LN_ERROR_INVALID_SWITCH_NUM;
    }
    switch_num -= 1;

    /*
     * Example output from JMRI for switch 2 from DT400:
     * [B0 01 10 5E]  Requesting Switch at LT2 to Thrown (Output On).
     * [B0 01 00 4E]  Requesting Switch at LT2 to Thrown (Output Off).
     * [B0 01 30 7E]  Requesting Switch at LT2 to Closed (Output On).
     * [B0 01 20 6E]  Requesting Switch at LT2 to Closed (Output Off).
     */
    msg.opcode = LN_OPC_SWITCH_REQUEST;
    msg.req_switch.sw1 = switch_num & 0x007F;
    msg.req_switch.sw2 = (switch_num & 0x1F00) >> 8;
    msg.req_switch.sw2 |= (0x01 << 4); /* output on */

    loconet_context_write_message(manager->parent, &msg);

    if(flags & LOCONET_TURNOUT_FLAG_OUTPUT_ON_ONLY){
        msg.req_switch.sw2 &= ~(0x01 << 4); /* output off */
        loconet_context_write_message(manager->parent, &msg);
    }

    return LN_OK;
}

int loconet_turnout_manager_close(struct loconet_turnout_manager* manager, int switch_num, int flags){
    struct loconet_message msg;

    if(switch_num > 2048 || switch_num < 1){
        return LN_ERROR_INVALID_SWITCH_NUM;
    }
    switch_num -= 1;
    msg.opcode = LN_OPC_SWITCH_REQUEST;
    msg.req_switch.sw1 = switch_num & 0x007F;
    msg.req_switch.sw2 = (switch_num & 0x1F00) >> 8;
    msg.req_switch.sw2 |= (0x01 << 5);
    msg.req_switch.sw2 |= (0x01 << 4); /* output on */

    loconet_context_write_message(manager->parent, &msg);

    if(flags & LOCONET_TURNOUT_FLAG_OUTPUT_ON_ONLY){
        msg.req_switch.sw2 &= ~(0x01 << 4); /* output off */
        loconet_context_write_message(manager->parent, &msg);
    }

    return LN_OK;
}

int loconet_turnout_manager_set_turnout_state_changed_callback(struct loconet_turnout_manager* manager, loconet_turnout_changed_function fn){
    if(!manager) return LN_ERROR_INVALID_ARG;
    manager->switch_changed_callback = fn;
    return LN_OK;
}

enum loconet_turnout_status loconet_turnout_manager_get_cached_turnout_state(struct loconet_turnout_manager* manager, int switch_num){
    if(switch_num <= 1 || switch_num > 2048){
        return LOCONET_SWITCH_UNKNOWN;
    }
    switch_num -= 1;
    int status_offset = switch_num / 4;
    int switch_offset = switch_num % 4;
    int status = manager->cached_switch_status[status_offset] & (0x3 << switch_offset);
    status = status >> switch_offset;

    if(status == 0x01){
        return LOCONET_SWITCH_CLOSED;
    }else if(status == 0x02){
        return LOCONET_SWITCH_THROWN;
    }

    return LOCONET_SWITCH_UNKNOWN;
}

void loconet_turnout_manager_set_userdata(struct loconet_turnout_manager* manager, void* user_data){
    if(!manager) return;
    manager->user_data = user_data;
}

void* loconet_turnout_manager_userdata(struct loconet_turnout_manager* manager){
    if(manager == NULL) return NULL;
    return manager->user_data;
}
