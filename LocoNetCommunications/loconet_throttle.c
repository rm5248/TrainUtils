#include <stdlib.h>
#include "loconet_throttle.h"
#include "loconet_buffer.h"

enum LocoRequestState{
    STATE_NONE,
    STATE_REQUEST,
    STATE_STEAL,
    STATE_NULL_MOVE,
    STATE_ACK,
    STATE_SET_COMMON,
};

struct loconet_throttle{
    struct loconet_context* parent;
    loconet_throttle_prompt_steal prompt_steal;
    loconet_throttle_locomotive_controlled locomotive_controlled;
    void* user_data;
    int locomotive_number;
    enum LocoRequestState request_state;
    uint16_t request_loco_number;
    uint8_t request_slot_number;
    uint8_t speed;
    uint8_t slot_number;
};

struct loconet_throttle* loconet_throttle_new(struct loconet_context* ctx){
    struct loconet_throttle* throttle = calloc(1, sizeof(struct loconet_throttle));
    throttle->parent = ctx;
    throttle->slot_number = 0xFF;

    return throttle;
}

void loconet_throttle_free(struct loconet_throttle* throttle){
    if(!throttle) return;

    free(throttle);
}

void loconet_throttle_incoming_message(struct loconet_throttle* throttle, struct loconet_message* msg){
    struct loconet_message outgoing_message;

    if( msg->opcode == LN_OPC_SLOT_READ_DATA ){
        int addr = msg->slot_data.addr1 | (msg->slot_data.addr2 << 7);
        if( throttle->request_state == STATE_REQUEST &&
                throttle->request_loco_number == addr ){
            if( LN_SLOT_STATUS((*msg)) == LN_SLOT_STATUS_IN_USE ){
                // Ask to steal
                throttle->request_state = STATE_STEAL;
                throttle->request_slot_number = msg->slot_data.slot;
                if(throttle->prompt_steal){
                    throttle->prompt_steal(throttle);
                }
            }else{
                //perform a NULL MOVE
                outgoing_message.opcode = LN_OPC_MOVE_SLOT;
                outgoing_message.move_slot.source = msg->slot_data.slot;
                outgoing_message.move_slot.slot = msg->slot_data.slot;
                throttle->request_state = STATE_NULL_MOVE;
                throttle->slot_number = msg->slot_data.slot;
                loconet_context_write_message(throttle->parent, &outgoing_message);
            }
        }

        if( throttle->request_state == STATE_NULL_MOVE &&
                throttle->request_loco_number == addr ){
            // We have successfully done the NULL MOVE! :D
            throttle->locomotive_number = throttle->request_loco_number;
        }

        if( throttle->request_state == STATE_NONE &&
                throttle->slot_number == msg->slot_data.slot ){
            // Update everything about this guy
        }
    }else if( msg->opcode == LN_OPC_LONG_ACK ){
        if( throttle->request_state == STATE_NULL_MOVE ){
            throttle->request_state = STATE_NONE;
        }
    }else if( msg->opcode == LN_OPC_LOCO_SPEED ){
        int realSpeed = msg->speed.speed - 1;
        if( realSpeed < 0 ){
            realSpeed = 0;
        }
        throttle->speed = realSpeed & 0xFF;
    }else if( msg->opcode == LN_OPC_LOCO_DIR_FUNC ){
        if( throttle->slot_number == msg->direction_functions.slot ){
            // TODO update anybody who is interested with the new function values
        }
    }else if( msg->opcode == LN_OPC_SLOT_STAT1 ){
        if( throttle->slot_number == msg->direction_functions.slot &&
                throttle->request_state == STATE_SET_COMMON ){
            // Dispatch the LOCO
            struct loconet_message msg;
            msg.opcode = LN_OPC_MOVE_SLOT;
            msg.move_slot.source = throttle->slot_number;
            msg.move_slot.slot = 0;
            loconet_context_write_message(throttle->parent, &msg);

            throttle->slot_number = 255;
            throttle->request_state = STATE_NONE;
        }
    }else if( msg->opcode == LN_OPC_LOCO_SOUND ){
        if( throttle->slot_number == msg->direction_functions.slot ){
            // TODO update anybody who is interested with the new function values
        }
    }
}

int loconet_throttle_set_speed(struct loconet_throttle* throttle, int speed){
    if(!throttle) return LN_ERROR_INVALID_ARG;

    if(speed < 0 || speed > 128){
        return LN_ERROR_INVALID_ARG;
    }

    if(throttle->slot_number == 0xFF){
        return LN_ERROR_NO_LOCO_SELECTED;
    }

    struct loconet_message message;
    message.opcode = LN_OPC_LOCO_SPEED;
    message.speed.speed = speed & 0x7F;

    throttle->speed = speed & 0x7F;

    /*
     * Special logic for Digitrax : speed step 0 is stopped,
     * while speed step 1 is ESTOP.
     * So, add 1 to each speed we get passed in.
     * if the speed is 1, that means we are now stopped
     */
    message.speed.speed++;
    if( message.speed.speed == 1 ){
        message.speed.speed = 0;
    }

    message.speed.slot = throttle->slot_number;
    loconet_context_write_message(throttle->parent, &message);

    return LN_OK;
}

int loconet_throttle_estop(struct loconet_throttle* throttle){
    if(!throttle) return LN_ERROR_INVALID_ARG;

    if(throttle->slot_number == 0xFF){
        return LN_ERROR_NO_LOCO_SELECTED;
    }

    struct loconet_message message;
    message.opcode = LN_OPC_LOCO_SPEED;
    message.speed.speed = 1;
    message.speed.slot = throttle->slot_number;
    throttle->speed = 1;
    loconet_context_write_message(throttle->parent, &message);

    return LN_OK;
}

int loconet_throttle_set_function(struct loconet_throttle* throttle, int function, int on){
    if(!throttle) return LN_ERROR_INVALID_ARG;


    return LN_OK;
}

int loconet_throttle_select_locomotive(struct loconet_throttle* throttle, int locomotive_number, int select_flags){
    if(!throttle) return LN_ERROR_INVALID_ARG;
    if(throttle->slot_number != 0xFF){
        return LN_ERROR_LOCO_ALREADY_SELECTED;
    }
    if(locomotive_number < 0 || locomotive_number > 10239){
        return LN_ERROR_INVALID_LOCO_NUMBER;
    }

    struct loconet_message msg;
    msg.opcode = LN_OPC_LOCO_ADDR;
    msg.addr.locoAddrLo = locomotive_number & 0x7F;
    msg.addr.locoAddrHi = (locomotive_number & ~0x7F) >> 7;
    throttle->request_state = STATE_REQUEST;
    throttle->request_loco_number = locomotive_number;

    loconet_context_write_message(throttle->parent, &msg);

    return LN_OK;
}

int loconet_throttle_set_prompt_steal_callback(struct loconet_throttle* throttle, loconet_throttle_prompt_steal cb){
    if(!throttle) return LN_ERROR_INVALID_ARG;
    throttle->prompt_steal = cb;
    return LN_OK;
}

int loconet_throttle_set_locomotive_controlled_callback(struct loconet_throttle* throttle, loconet_throttle_locomotive_controlled cb ){
    if(!throttle) return LN_ERROR_INVALID_ARG;
    throttle->locomotive_controlled = cb;
    return LN_OK;
}

int loconet_throttle_confirm_steal(struct loconet_throttle* throttle){
    if( throttle->request_state == STATE_STEAL ){
        throttle->slot_number = throttle->request_slot_number;
        // Request the slot data so that we update our status
        struct loconet_message msg;
        msg.opcode = LN_OPC_REQUEST_SLOT_DATA;
        msg.req_slot_data.slot = throttle->slot_number;
        msg.req_slot_data.nul = 0;
        loconet_context_write_message(throttle->parent, &msg);
    }
    throttle->request_state = STATE_NONE;
    return LN_OK;
}

int loconet_throttle_abort_loco_selection(struct loconet_throttle* throttle){
    throttle->request_state = STATE_NONE;
    return LN_OK;
}

int loconet_throttle_slot_update(struct loconet_throttle* throttle){
    if(!throttle) return LN_ERROR_INVALID_ARG;

    if(throttle->slot_number == 0xFF){
        return LN_ERROR_NO_LOCO_SELECTED;
    }

    struct loconet_message message;
    message.opcode = LN_OPC_LOCO_SPEED;
    message.speed.speed = throttle->speed;
    message.speed.slot = throttle->slot_number;
    loconet_context_write_message(throttle->parent, &message);

    return LN_OK;
}

int loconet_throttle_dispatch(struct loconet_throttle* throttle){
    if(!throttle) return LN_ERROR_INVALID_ARG;

    if(throttle->slot_number == 0xFF){
        return LN_ERROR_NO_LOCO_SELECTED;
    }

    // Two step process:
    // 1. set slot to COMMON
    // 2. Move to slot 0(DISPATCH)
    // Dispatch the slot
    throttle->request_state = STATE_SET_COMMON;

    struct loconet_message msg;
    msg.opcode = LN_OPC_SLOT_STAT1;
    msg.stat1.slot = throttle->slot_number;
    msg.stat1.stat1 = (LN_SLOT_STATUS_COMMON << 4) | 0x7;
    loconet_context_write_message(throttle->parent, &msg);

    return LN_OK;
}

int loconet_throttle_get_locomotive_number(struct loconet_throttle* throttle){
    if(!throttle) return LN_ERROR_INVALID_ARG;
    return throttle->locomotive_number;
}

int loconet_throttle_set_userdata(struct loconet_throttle* throttle, void* user_data){
    if(!throttle) return LN_ERROR_INVALID_ARG;
    throttle->user_data = user_data;
    return LN_OK;
}

void* loconet_throttle_userdata(struct loconet_throttle* throttle){
    if(!throttle) return NULL;
    return throttle->user_data;
}
