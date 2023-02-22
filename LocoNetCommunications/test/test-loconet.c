/* SPDX-License-Identifier: GPL-2.0 */
#include <string.h>

#include "loconet_buffer.h"
#include "loconet_sensor.h"

static struct loconet_sensor_manager* sensor_mgr;
static struct loconet_message incoming_messages[10];
static int incoming_messages_pos = 0;

static void loconet_incoming(struct loconet_context* ctx, struct loconet_message* message){
    incoming_messages[incoming_messages_pos] = *message;
    incoming_messages_pos++;

    if(sensor_mgr){
        loconet_sensor_manager_incoming_message(sensor_mgr, message);
    }
}

static int loconet_parse_speed(){
    // JMRI decoded: [A0 08 25 72]  Set speed of loco in slot 8 to 37.
    const uint8_t bytes[] = {0xA0, 0x08, 0x25, 0x72};

    struct loconet_context* ctx = loconet_context_new_interlocked(NULL);

    loconet_context_set_message_callback(ctx, loconet_incoming);
    loconet_context_incoming_bytes(ctx, bytes, sizeof(bytes));
    loconet_context_process(ctx);

    loconet_context_free(ctx);

    if(incoming_messages_pos != 1){
        return 1;
    }

    if(incoming_messages[0].speed.slot != 8){
        return 1;
    }

    if(incoming_messages[0].speed.speed != 37){
        return 1;
    }

    return 0;
}

static enum loconet_sensor_status loconet_test_sensor_parse(const uint8_t bytes[4], int sensor_num){
    enum loconet_sensor_status stat = LOCONET_SENSOR_UNKNOWN;
    struct loconet_context* ctx = loconet_context_new_interlocked(NULL);
    sensor_mgr = loconet_sensor_manager_new(ctx);

    loconet_context_set_message_callback(ctx, loconet_incoming);
    loconet_context_incoming_bytes(ctx, bytes, 4);
    loconet_context_process(ctx);

    if(incoming_messages_pos != 1){
        goto out;
    }

    stat = loconet_sensor_manager_get_cached_sensor_state(sensor_mgr, sensor_num);

out:
    loconet_sensor_manager_free(sensor_mgr);
    loconet_context_free(ctx);

    return stat;
}

static int loconet_parse_sensor(){
    // JMRI decoded: [B2 3A 50 27]  Sensor LS117 () is High.  (BDL16 # 8, DS5; DS54/DS64/SE8c # 15, AuxC/A3/DS05).
    const uint8_t bytes[] = {0xB2, 0x3A, 0x50, 0x27};
    int ret = 0;

    if(loconet_test_sensor_parse(bytes, 117) != LOCONET_SENSOR_ACTIVE){
        ret = 1;
    }

    return ret;
}

static int loconet_parse_sensor2(){
    // JMRI decoded: [B2 5D 62 72]  Sensor LS700 () is Low.  (BDL16 # 44, DS12; DS54/DS64 # 88, SwiB/S2).
    const uint8_t bytes[] = {0xB2, 0x5D, 0x62, 0x72};
    int ret = 0;

    if(loconet_test_sensor_parse(bytes, 700) != LOCONET_SENSOR_INACTIVE){
        ret = 1;
    }

    return ret;
}

static int loconet_parse_turnout(){
    return 1;
}

int main(int argc, char** argv){
    if(argc == 1){
        return 1;
    }

    if(strcmp(argv[1], "parse_speed") == 0){
        return loconet_parse_speed();
    }else if(strcmp(argv[1], "parse_sensor") == 0){
        return loconet_parse_sensor();
    }else if(strcmp(argv[1], "parse_sensor2") == 0){
        return loconet_parse_sensor2();
    }else if(strcmp(argv[1], "parse_turnout") == 0){
        return loconet_parse_turnout();
    }

	return 1;
}
