#include "loconet_util.h"
#include "loconet_buffer.h"

int loconet_global_power_on(struct loconet_context* ctx){
    if(!ctx){
        return LN_ERROR_INVALID_ARG;
    }

    struct loconet_message msg;
    msg.opcode = LN_OPC_POWER_ON;
    return loconet_context_write_message(ctx, &msg);
}

int loconet_global_power_off(struct loconet_context* ctx){
    if(!ctx){
        return LN_ERROR_INVALID_ARG;
    }

    struct loconet_message msg;
    msg.opcode = LN_OPC_POWER_OFF;
    return loconet_context_write_message(ctx, &msg);
}
