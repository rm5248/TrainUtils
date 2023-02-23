/* SPDX-License-Identifier: GPL-2.0 */
#include <string.h>

#include "loconet_buffer.h"
#include "loconet_sensor.h"
#include "loconet_turnout.h"

static struct loconet_sensor_manager* sensor_mgr;
static struct loconet_turnout_manager* turnout_mgr;
static struct loconet_message incoming_messages[10];
static int incoming_messages_pos = 0;

static void loconet_incoming(struct loconet_context* ctx, struct loconet_message* message){
    incoming_messages[incoming_messages_pos] = *message;
    incoming_messages_pos++;

    if(sensor_mgr){
        loconet_sensor_manager_incoming_message(sensor_mgr, message);
    }
    if(turnout_mgr){
        loconet_turnout_manager_incoming_message(turnout_mgr, message);
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

static enum loconet_turnout_status loconet_test_turnout_parse(const uint8_t bytes[4], int turnout_num){
    enum loconet_turnout_status stat = LOCONET_TURNOUT_UNKNOWN;
    struct loconet_context* ctx = loconet_context_new_interlocked(NULL);
    turnout_mgr = loconet_turnout_manager_new(ctx);

    loconet_context_set_message_callback(ctx, loconet_incoming);
    loconet_context_incoming_bytes(ctx, bytes, 4);
    loconet_context_process(ctx);

    if(incoming_messages_pos != 1){
        goto out;
    }

    stat = loconet_turnout_manager_get_cached_turnout_state(turnout_mgr, turnout_num);

out:
    loconet_turnout_manager_free(turnout_mgr);
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

static int loconet_parse_turnout1(){
    /*
     * [B0 21 10 7E]  Requesting Switch at LT34 to Thrown (Output On).
     * [B0 21 00 6E]  Requesting Switch at LT34 to Thrown (Output Off).
     * [B0 21 30 5E]  Requesting Switch at LT34 to Closed (Output On).
     * [B0 21 20 4E]  Requesting Switch at LT34 to Closed (Output Off).
     */
    const uint8_t bytes[] = {0xB0, 0x21, 0x10, 0x7E};
    int ret = 0;

    if(loconet_test_turnout_parse(bytes, 34) != LOCONET_TURNOUT_THROWN){
        ret = 1;
    }

    return ret;
}

static int loconet_parse_turnout2(){
    const uint8_t bytes[] = {0xB0, 0x21, 0x20, 0x4E};
    int ret = 0;

    if(loconet_test_turnout_parse(bytes, 34) != LOCONET_TURNOUT_CLOSED){
        ret = 1;
    }

    return ret;
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
    }else if(strcmp(argv[1], "parse_turnout1") == 0){
        return loconet_parse_turnout1();
    }else if(strcmp(argv[1], "parse_turnout2") == 0){
        return loconet_parse_turnout2();
    }

	return 1;
}
