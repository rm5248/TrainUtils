/* SPDX-License-Identifier: GPL-2.0 */
#include <string.h>
#include <stdio.h>

#include "lcc.h"
#include "lcc-common.h"
#include "lcc-datagram.h"
#include "lcc-memory.h"
#include "lcc-remote-memory.h"
#include "test-common.h"

const char* cdi_data = "abcdefghijklmnopqrstuvwxyz";

struct memory_cb_info{
    int ok;
    uint16_t alias;
    uint16_t errcode;
};

static int null_read_fn(struct lcc_memory_context* ctx, uint16_t alias, uint8_t address_space, uint32_t starting_address, uint8_t read_count){
    printf("Read %d bytes from address space 0x%02X\n", read_count, address_space);
    return LCC_OK;
}

static void write_memory_response_to_buffer(struct lcc_remote_memory_context* ctx, uint16_t alias, uint8_t address_space, uint32_t starting_address, void* memory_data, int len){
    void* user_data = lcc_context_user_data(lcc_remote_memory_parent(ctx));
    memcpy(user_data, memory_data, len);
}

static void memory_request_ok_cb(struct lcc_remote_memory_context* ctx, uint16_t alias, uint8_t flags){
    struct memory_cb_info* user_data = lcc_context_user_data(lcc_remote_memory_parent(ctx));
    user_data->ok = 1;
    user_data->alias = alias;
    user_data->errcode = 0;
}

static void memory_request_fail_cb(struct lcc_remote_memory_context* ctx, uint16_t alias, uint16_t error_code, void* optional_data, int optional_len){
    struct memory_cb_info* user_data = lcc_context_user_data(lcc_remote_memory_parent(ctx));
    user_data->ok = 0;
    user_data->alias = alias;
    user_data->errcode = error_code;
}

static int test_datagram(){
    struct lcc_context** both_ctx = lcctest_create_contexts(2);

    // One of our contexts has datagram data(both_ctx[0]).
    // We will request it from the other context(both_ctx[1]).
    struct lcc_datagram_context* datagram_ctx1 = lcc_datagram_context_new(both_ctx[0]);
    struct lcc_memory_context* memory_ctx1 = lcc_memory_new(both_ctx[0]);
    struct lcc_datagram_context* datagram_ctx2 = lcc_datagram_context_new(both_ctx[1]);
    struct lcc_remote_memory_context* remote_memory_ctx2 = lcc_remote_memory_new(both_ctx[1]);

    lcc_memory_set_cdi(memory_ctx1, cdi_data, strlen(cdi_data), 0);

    char buffer[128] = {0};
    lcc_context_set_userdata(both_ctx[1], buffer);
    lcc_remote_memory_set_functions(remote_memory_ctx2,
                                    NULL,
                                    NULL,
                                    write_memory_response_to_buffer,
                                    NULL);
    lcc_remote_memory_read_single_transfer(remote_memory_ctx2,
            lcc_context_alias(both_ctx[0]),
            LCC_MEMORY_SPACE_CONFIGURATION_DEFINITION,
            0,
            26);

    lcctest_pump_frames();

    if(memcmp(buffer, cdi_data, strlen(cdi_data)) == 0){
        return 0;
    }

    return 1;
}

// Test to make sure that the receiving node will only process
// one datagram at a time and send back a rejection if we try to get it to
// send another one before we are finished sending the first
// NOTE: Not sure if we can actually do this test??
// We would need to be able to interleave CAN packets, and this is only a problem for writes I think
// since this code needs to test INCOMING messages to the node.  Outgoing messages
// are done at a single time
#if 0
static int test_datagram_same_time(){
    struct lcc_context** both_ctx = lcctest_create_contexts(3);

    // One of our contexts has datagram data(both_ctx[0]).
    // We will request it from both of the other contexts(both_ctx[1], both_ctx[2]).
    struct lcc_datagram_context* datagram_ctx1 = lcc_datagram_context_new(both_ctx[0]);
    struct lcc_memory_context* memory_ctx1 = lcc_memory_new(both_ctx[0]);
    struct lcc_datagram_context* datagram_ctx2 = lcc_datagram_context_new(both_ctx[1]);
    struct lcc_remote_memory_context* memory_ctx2 = lcc_remote_memory_new(both_ctx[1]);
    struct lcc_datagram_context* datagram_ctx3 = lcc_datagram_context_new(both_ctx[2]);
    struct lcc_remote_memory_context* memory_ctx3 = lcc_remote_memory_new(both_ctx[2]);

    lcc_memory_set_cdi(memory_ctx1, cdi_data, strlen(cdi_data), 0);
    lcc_memory_set_memory_functions(memory_ctx1,
                                    NULL,
                                    null_read_fn,
                                    NULL);

    struct memory_cb_info cb1 = {0};
    struct memory_cb_info cb2 = {0};
    lcc_context_set_userdata(both_ctx[1], &cb1);
    lcc_remote_memory_read_single_transfer(memory_ctx2,
            lcc_context_alias(both_ctx[0]),
            LCC_MEMORY_SPACE_ALL_MEMORY,
            0,
            50);
    lcc_remote_memory_set_functions(memory_ctx2,
                                    memory_request_ok_cb,
                                    memory_request_fail_cb,
                                    NULL,
                                    NULL);

    lcc_context_set_userdata(both_ctx[2], &cb2);
    lcc_remote_memory_read_single_transfer(memory_ctx3,
            lcc_context_alias(both_ctx[0]),
            LCC_MEMORY_SPACE_ALL_MEMORY,
            0,
            50);
    lcc_remote_memory_set_functions(memory_ctx3,
                                    memory_request_ok_cb,
                                    memory_request_fail_cb,
                                    NULL,
                                    NULL);

    lcctest_pump_frames();

    printf("HI");
    return 1;
}
#endif

int main(int argc, char** argv){
    if(argc < 2) return 1;

    if(strcmp(argv[1], "datagram") == 0){
        return test_datagram();
    }else if(strcmp(argv[1], "datagram-same-time") == 0){
//        return test_datagram_same_time();
    }

    return 1;
}
