/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Tests for reading memory between two nodes.
 *
 * Node 0 provides its memory using an lcc_memory_map(plus a CDI), node 1
 * queries and reads that memory using the remote memory functions.
 */
#include <string.h>
#include <stdio.h>

#include "lcc.h"
#include "lcc-common.h"
#include "lcc-datagram.h"
#include "lcc-memory.h"
#include "lcc-memory-map.h"
#include "lcc-remote-memory.h"
#include "test-common.h"

#define CHECK(cond) do{ \
    if(!(cond)){ \
        printf("%s:%d: check failed: %s\n", __FILE__, __LINE__, #cond); \
        return 1; \
    } \
}while(0)

#define SPACE_ACDI_MANUFACTURER 0xFC
#define SPACE_ACDI_USER 0xFB
// A space that is not one of the 'special' spaces, and does not start at address 0
#define SPACE_GENERIC 0x10
#define SPACE_GENERIC_LOW_ADDRESS 0x100
// A space that the node does not have
#define SPACE_MISSING 0x20

// Longer than 64 bytes, so that reading it all takes more than one read
static const char* cdi_data =
        "<?xml version='1.0'?>"
        "<cdi><identification><manufacturer>TrainUtils</manufacturer>"
        "<model>Memory test</model></identification></cdi>";

static uint8_t config_memory[128];
static uint8_t acdi_manufacturer_memory[125];
static uint8_t acdi_user_memory[128];
static uint8_t generic_memory[64];

static struct lcc_memory_segment segments[] = {
    { LCC_MEMORY_SPACE_CONFIGURATION_SPACE, LCC_MEMORY_FLAG_SPACE_WRITEABLE, 0, sizeof(config_memory) - 1, config_memory },
    { SPACE_ACDI_MANUFACTURER, LCC_MEMORY_FLAG_SPACE_READ_ONLY, 0, sizeof(acdi_manufacturer_memory) - 1, acdi_manufacturer_memory },
    { SPACE_ACDI_USER, LCC_MEMORY_FLAG_SPACE_WRITEABLE, 0, sizeof(acdi_user_memory) - 1, acdi_user_memory },
    { SPACE_GENERIC, LCC_MEMORY_FLAG_SPACE_WRITEABLE, SPACE_GENERIC_LOW_ADDRESS, SPACE_GENERIC_LOW_ADDRESS + sizeof(generic_memory) - 1, generic_memory },
    { 0 },
};

/*
 * Everything that node 1(the one doing the reading) has received
 */
struct remote_results{
    int info_count;
    int info_present;
    int info_readonly;
    uint8_t info_space;
    uint32_t info_low;
    uint32_t info_high;

    int read_count;
    uint8_t read_space;
    uint32_t read_address;
    uint8_t read_data[72];
    int read_len;

    int reject_count;
    uint8_t reject_space;
    uint32_t reject_address;
    uint16_t reject_errcode;

    int fail_count;
    uint16_t fail_errcode;

    int write_ok_count;
    uint8_t write_ok_space;
    uint32_t write_ok_address;

    int write_reject_count;
    uint8_t write_reject_space;
    uint32_t write_reject_address;
    uint16_t write_reject_errcode;
    int write_reject_has_message;

    // From the memory map on node 0
    int map_written_count;
    uint8_t map_written_space;
    uint32_t map_written_offset;
    uint32_t map_written_len;
};

static struct remote_results results;
static struct lcc_remote_memory_context* remote_ctx;
static uint16_t memory_node_alias;
// A node that accepts all datagrams without replying with a 'reply pending'
static uint16_t ack_only_node_alias;
// A node that does not handle datagrams at all, so it never responds to anything
static uint16_t silent_node_alias;

static void remote_request_ok(struct lcc_remote_memory_context* ctx, uint16_t alias, uint8_t flags){
}

static void remote_request_fail(struct lcc_remote_memory_context* ctx, uint16_t alias, uint16_t error_code, void* optional_data, int optional_len){
    results.fail_count++;
    results.fail_errcode = error_code;
}

static void remote_memory_received(struct lcc_remote_memory_context* ctx, uint16_t alias, uint8_t address_space, uint32_t starting_address, void* memory_data, int len){
    results.read_count++;
    results.read_space = address_space;
    results.read_address = starting_address;
    results.read_len = len;
    if(len > 0 && len <= (int)sizeof(results.read_data)){
        memcpy(results.read_data, memory_data, len);
    }
}

static void remote_read_rejected(struct lcc_remote_memory_context* ctx, uint16_t alias, uint8_t address_space, uint32_t starting_address, uint16_t error_code, const char* message){
    results.reject_count++;
    results.reject_space = address_space;
    results.reject_address = starting_address;
    results.reject_errcode = error_code;
}

static void remote_memory_info(struct lcc_remote_memory_context* ctx, uint16_t alias, int exists, int readonly, uint8_t address_space, uint32_t lowest_address, uint32_t highest_address, const char* message){
    results.info_count++;
    results.info_present = exists;
    results.info_readonly = readonly;
    results.info_space = address_space;
    results.info_low = lowest_address;
    results.info_high = highest_address;
}

static void remote_write_ok(struct lcc_remote_memory_context* ctx, uint16_t alias, uint8_t address_space, uint32_t starting_address){
    results.write_ok_count++;
    results.write_ok_space = address_space;
    results.write_ok_address = starting_address;
}

static void remote_write_rejected(struct lcc_remote_memory_context* ctx, uint16_t alias, uint8_t address_space, uint32_t starting_address, uint16_t error_code, const char* message){
    results.write_reject_count++;
    results.write_reject_space = address_space;
    results.write_reject_address = starting_address;
    results.write_reject_errcode = error_code;
    results.write_reject_has_message = message != NULL && message[0] != 0;
}

static void map_written(struct lcc_memory_map* map, uint8_t space, uint32_t offset, uint32_t len){
    results.map_written_count++;
    results.map_written_space = space;
    results.map_written_offset = offset;
    results.map_written_len = len;
}

static void ack_only_datagram(struct lcc_datagram_context* ctx, uint16_t alias, void* datagram_data, int len){
    // Accept the datagram, with no reply pending.  For a write, this means that the write is complete.
    lcc_datagram_respond_rxok(ctx, alias, 0);
}

static void setup(void){
    struct lcc_context** ctx = lcctest_create_contexts(4);

    for(int x = 0; x < (int)sizeof(config_memory); x++){
        config_memory[x] = x;
    }
    for(int x = 0; x < (int)sizeof(acdi_manufacturer_memory); x++){
        acdi_manufacturer_memory[x] = 0x40 + x;
    }
    for(int x = 0; x < (int)sizeof(acdi_user_memory); x++){
        acdi_user_memory[x] = 0x80 + x;
    }
    for(int x = 0; x < (int)sizeof(generic_memory); x++){
        generic_memory[x] = 0xFF - x;
    }

    // Node 0: has memory
    lcc_datagram_context_new(ctx[0]);
    struct lcc_memory_context* memory_ctx = lcc_memory_new(ctx[0]);
    lcc_memory_set_cdi(memory_ctx, (void*)cdi_data, strlen(cdi_data), 0);
    struct lcc_memory_map* map = lcc_memory_map_new(memory_ctx, segments);
    lcc_memory_map_set_written_callback(map, map_written);
    memory_node_alias = lcc_context_alias(ctx[0]);

    // Node 1: reads memory from node 0
    lcc_datagram_context_new(ctx[1]);
    remote_ctx = lcc_remote_memory_new(ctx[1]);
    lcc_remote_memory_set_functions(remote_ctx,
                                    remote_request_ok,
                                    remote_request_fail,
                                    remote_memory_received,
                                    remote_read_rejected,
                                    remote_memory_info);
    lcc_remote_memory_set_write_functions(remote_ctx,
                                          remote_write_ok,
                                          remote_write_rejected);

    // Node 2: accepts any datagram, but doesn't do anything with it
    struct lcc_datagram_context* ack_only_ctx = lcc_datagram_context_new(ctx[2]);
    lcc_datagram_context_set_datagram_functions(ack_only_ctx, ack_only_datagram, NULL, NULL);
    ack_only_node_alias = lcc_context_alias(ctx[2]);

    // Node 3: no datagram handling, so it ignores memory requests
    silent_node_alias = lcc_context_alias(ctx[3]);

    lcctest_pump_frames();
    memset(&results, 0, sizeof(results));
}

static int query_space(uint8_t space){
    memset(&results, 0, sizeof(results));
    int stat = lcc_remote_memory_get_address_space_information(remote_ctx, memory_node_alias, space);
    if(stat != LCC_OK){
        printf("Unable to send address space query: %d\n", stat);
        return stat;
    }
    lcctest_pump_frames();
    return LCC_OK;
}

static int read_space(uint8_t space, uint32_t address, int count){
    memset(&results, 0, sizeof(results));
    int stat = lcc_remote_memory_read_single_transfer(remote_ctx, memory_node_alias, space, address, count);
    if(stat != LCC_OK){
        printf("Unable to send memory read: %d\n", stat);
        return stat;
    }
    lcctest_pump_frames();
    return LCC_OK;
}

static int write_space_to(uint16_t alias, uint8_t space, uint32_t address, const void* data, int count){
    memset(&results, 0, sizeof(results));
    int stat = lcc_remote_memory_write_single_transfer(remote_ctx, alias, space, address, data, count);
    if(stat != LCC_OK){
        return stat;
    }
    lcctest_pump_frames();
    return LCC_OK;
}

static int write_space(uint8_t space, uint32_t address, const void* data, int count){
    return write_space_to(memory_node_alias, space, address, data, count);
}

/*
 * Check that the last write was successful
 */
static int check_write_ok(uint8_t space, uint32_t address){
    CHECK(results.fail_count == 0);
    CHECK(results.write_reject_count == 0);
    CHECK(results.write_ok_count == 1);
    CHECK(results.write_ok_space == space);
    CHECK(results.write_ok_address == address);
    return 0;
}

/*
 * Check that the last write was rejected by the node with the given error
 */
static int check_write_rejected(uint8_t space, uint32_t address, uint16_t errcode){
    CHECK(results.fail_count == 0);
    CHECK(results.write_ok_count == 0);
    CHECK(results.write_reject_count == 1);
    CHECK(results.write_reject_space == space);
    CHECK(results.write_reject_address == address);
    CHECK(results.write_reject_errcode == errcode);
    // The node did not send a message along with the error
    CHECK(!results.write_reject_has_message);
    CHECK(results.map_written_count == 0);
    return 0;
}

/*
 * Check that the last read was successful and returned the given data
 */
static int check_read(uint8_t space, uint32_t address, const uint8_t* expected, int len){
    CHECK(results.fail_count == 0);
    CHECK(results.reject_count == 0);
    CHECK(results.read_count == 1);
    CHECK(results.read_space == space);
    CHECK(results.read_address == address);
    CHECK(results.read_len == len);
    CHECK(memcmp(results.read_data, expected, len) == 0);
    return 0;
}

static int test_info_config_space(void){
    setup();

    CHECK(query_space(LCC_MEMORY_SPACE_CONFIGURATION_SPACE) == LCC_OK);
    CHECK(results.fail_count == 0);
    CHECK(results.info_count == 1);
    CHECK(results.info_present);
    CHECK(!results.info_readonly);
    CHECK(results.info_space == LCC_MEMORY_SPACE_CONFIGURATION_SPACE);
    CHECK(results.info_low == 0);
    CHECK(results.info_high == sizeof(config_memory) - 1);

    return 0;
}

static int test_info_readonly(void){
    setup();

    CHECK(query_space(SPACE_ACDI_MANUFACTURER) == LCC_OK);
    CHECK(results.fail_count == 0);
    CHECK(results.info_count == 1);
    CHECK(results.info_present);
    CHECK(results.info_readonly);
    CHECK(results.info_space == SPACE_ACDI_MANUFACTURER);
    CHECK(results.info_high == sizeof(acdi_manufacturer_memory) - 1);

    return 0;
}

static int test_info_low_address(void){
    setup();

    CHECK(query_space(SPACE_GENERIC) == LCC_OK);
    CHECK(results.fail_count == 0);
    CHECK(results.info_count == 1);
    CHECK(results.info_present);
    CHECK(results.info_space == SPACE_GENERIC);
    CHECK(results.info_low == SPACE_GENERIC_LOW_ADDRESS);
    CHECK(results.info_high == SPACE_GENERIC_LOW_ADDRESS + sizeof(generic_memory) - 1);

    return 0;
}

static int test_info_missing_space(void){
    setup();

    CHECK(query_space(SPACE_MISSING) == LCC_OK);
    CHECK(results.fail_count == 0);
    CHECK(results.info_count == 1);
    CHECK(!results.info_present);
    CHECK(results.info_space == SPACE_MISSING);

    return 0;
}

/*
 * The highest address of a space is the address of the last byte in it.  The CDI
 * ends with a terminating NULL, so the last byte is the NULL.
 */
static int test_info_cdi(void){
    setup();

    CHECK(query_space(LCC_MEMORY_SPACE_CONFIGURATION_DEFINITION) == LCC_OK);
    CHECK(results.fail_count == 0);
    CHECK(results.info_count == 1);
    CHECK(results.info_present);
    CHECK(results.info_space == LCC_MEMORY_SPACE_CONFIGURATION_DEFINITION);
    CHECK(results.info_low == 0);
    CHECK(results.info_high == strlen(cdi_data));

    return 0;
}

/*
 * The byte right after the CDI text must be the terminating NULL
 */
static int test_read_cdi_null(void){
    const uint8_t null_byte = 0;

    setup();

    CHECK(read_space(LCC_MEMORY_SPACE_CONFIGURATION_DEFINITION, strlen(cdi_data), 1) == LCC_OK);
    CHECK(check_read(LCC_MEMORY_SPACE_CONFIGURATION_DEFINITION, strlen(cdi_data), &null_byte, 1) == 0);

    return 0;
}

static int test_read_cdi(void){
    setup();

    CHECK(read_space(LCC_MEMORY_SPACE_CONFIGURATION_DEFINITION, 0, 64) == LCC_OK);
    CHECK(check_read(LCC_MEMORY_SPACE_CONFIGURATION_DEFINITION, 0, (const uint8_t*)cdi_data, 64) == 0);

    return 0;
}

static int test_read_config_space(void){
    setup();

    CHECK(read_space(LCC_MEMORY_SPACE_CONFIGURATION_SPACE, 0, 64) == LCC_OK);
    CHECK(check_read(LCC_MEMORY_SPACE_CONFIGURATION_SPACE, 0, config_memory, 64) == 0);

    CHECK(read_space(LCC_MEMORY_SPACE_CONFIGURATION_SPACE, 64, 64) == LCC_OK);
    CHECK(check_read(LCC_MEMORY_SPACE_CONFIGURATION_SPACE, 64, config_memory + 64, 64) == 0);

    return 0;
}

/*
 * Read from a space that needs the space number in the datagram(i.e. not 0xFD-0xFF)
 */
static int test_read_generic_space(void){
    setup();

    CHECK(read_space(SPACE_ACDI_USER, 0, 64) == LCC_OK);
    CHECK(check_read(SPACE_ACDI_USER, 0, acdi_user_memory, 64) == 0);

    CHECK(read_space(SPACE_ACDI_USER, 64, 10) == LCC_OK);
    CHECK(check_read(SPACE_ACDI_USER, 64, acdi_user_memory + 64, 10) == 0);

    return 0;
}

static int test_read_generic_low_address(void){
    setup();

    CHECK(read_space(SPACE_GENERIC, SPACE_GENERIC_LOW_ADDRESS, 64) == LCC_OK);
    CHECK(check_read(SPACE_GENERIC, SPACE_GENERIC_LOW_ADDRESS, generic_memory, 64) == 0);

    CHECK(read_space(SPACE_GENERIC, SPACE_GENERIC_LOW_ADDRESS + 60, 4) == LCC_OK);
    CHECK(check_read(SPACE_GENERIC, SPACE_GENERIC_LOW_ADDRESS + 60, generic_memory + 60, 4) == 0);

    return 0;
}

static int test_read_missing_space(void){
    setup();

    CHECK(read_space(SPACE_MISSING, 0, 16) == LCC_OK);
    CHECK(results.fail_count == 0);
    CHECK(results.read_count == 0);
    CHECK(results.reject_count == 1);
    CHECK(results.reject_space == SPACE_MISSING);
    CHECK(results.reject_errcode == LCC_ERRCODE_NOT_IMPLEMENTED);

    // A rejected read must not prevent further reads
    CHECK(read_space(LCC_MEMORY_SPACE_CONFIGURATION_SPACE, 0, 16) == LCC_OK);
    CHECK(check_read(LCC_MEMORY_SPACE_CONFIGURATION_SPACE, 0, config_memory, 16) == 0);

    return 0;
}

static int test_read_out_of_range(void){
    setup();

    // Starts inside of the space, but goes past the end
    CHECK(read_space(LCC_MEMORY_SPACE_CONFIGURATION_SPACE, sizeof(config_memory) - 10, 20) == LCC_OK);
    CHECK(results.fail_count == 0);
    CHECK(results.read_count == 0);
    CHECK(results.reject_count == 1);
    CHECK(results.reject_space == LCC_MEMORY_SPACE_CONFIGURATION_SPACE);
    CHECK(results.reject_errcode == LCC_ERRCODE_INVALID_ARG);

    // Below the low address
    CHECK(read_space(SPACE_GENERIC, 0, 16) == LCC_OK);
    CHECK(results.reject_count == 1);
    CHECK(results.reject_space == SPACE_GENERIC);
    CHECK(results.reject_errcode == LCC_ERRCODE_INVALID_ARG);

    return 0;
}

/*
 * Do what a configuration tool does: query the space for its bounds,
 * then read the entire thing in 64 byte chunks.
 */
static int read_entire_space(uint8_t space, const uint8_t* expected, uint32_t expected_len){
    uint8_t space_data[512];
    uint32_t total_read = 0;

    CHECK(query_space(space) == LCC_OK);
    CHECK(results.info_count == 1);
    CHECK(results.info_present);

    uint32_t low = results.info_low;
    uint32_t high = results.info_high;
    // The highest address is the address of the last byte in the space
    uint32_t space_len = high - low + 1;
    CHECK(space_len == expected_len);
    CHECK(space_len <= sizeof(space_data));

    while(total_read < space_len){
        int count = space_len - total_read;
        if(count > 64){
            count = 64;
        }
        CHECK(read_space(space, low + total_read, count) == LCC_OK);
        CHECK(results.reject_count == 0);
        CHECK(results.read_count == 1);
        CHECK(results.read_len == count);
        memcpy(space_data + total_read, results.read_data, count);
        total_read += count;
    }

    CHECK(memcmp(space_data, expected, expected_len) == 0);

    return 0;
}

static int test_read_entire_space(void){
    setup();

    return read_entire_space(SPACE_GENERIC, generic_memory, sizeof(generic_memory));
}

static int test_read_entire_cdi(void){
    setup();

    // The terminating NULL must be sent as part of the CDI
    return read_entire_space(LCC_MEMORY_SPACE_CONFIGURATION_DEFINITION, (const uint8_t*)cdi_data, strlen(cdi_data) + 1);
}

static int test_write_config_space(void){
    const uint8_t data[] = { 0xDE, 0xAD, 0xBE, 0xEF };

    setup();

    CHECK(write_space(LCC_MEMORY_SPACE_CONFIGURATION_SPACE, 10, data, sizeof(data)) == LCC_OK);
    CHECK(check_write_ok(LCC_MEMORY_SPACE_CONFIGURATION_SPACE, 10) == 0);
    CHECK(memcmp(config_memory + 10, data, sizeof(data)) == 0);
    // Surrounding memory is untouched
    CHECK(config_memory[9] == 9);
    CHECK(config_memory[14] == 14);

    CHECK(results.map_written_count == 1);
    CHECK(results.map_written_space == LCC_MEMORY_SPACE_CONFIGURATION_SPACE);
    CHECK(results.map_written_offset == 10);
    CHECK(results.map_written_len == sizeof(data));

    return 0;
}

/*
 * Write to a space that needs the space number in the datagram(i.e. not 0xFD-0xFF)
 */
static int test_write_generic_space(void){
    const uint8_t data[] = { 'T', 'e', 's', 't', 0 };

    setup();

    CHECK(write_space(SPACE_ACDI_USER, 1, data, sizeof(data)) == LCC_OK);
    CHECK(check_write_ok(SPACE_ACDI_USER, 1) == 0);
    CHECK(memcmp(acdi_user_memory + 1, data, sizeof(data)) == 0);
    CHECK(acdi_user_memory[0] == 0x80);

    return 0;
}

static int test_write_generic_low_address(void){
    const uint8_t data[] = { 0x01, 0x02 };

    setup();

    CHECK(write_space(SPACE_GENERIC, SPACE_GENERIC_LOW_ADDRESS + 8, data, sizeof(data)) == LCC_OK);
    CHECK(check_write_ok(SPACE_GENERIC, SPACE_GENERIC_LOW_ADDRESS + 8) == 0);
    CHECK(memcmp(generic_memory + 8, data, sizeof(data)) == 0);

    return 0;
}

/*
 * 64 bytes is the most that can be written at once
 */
static int test_write_max_size(void){
    uint8_t data[64];
    for(int x = 0; x < (int)sizeof(data); x++){
        data[x] = 0xA0 ^ x;
    }

    setup();

    CHECK(write_space(SPACE_ACDI_USER, 64, data, sizeof(data)) == LCC_OK);
    CHECK(check_write_ok(SPACE_ACDI_USER, 64) == 0);
    CHECK(memcmp(acdi_user_memory + 64, data, sizeof(data)) == 0);

    return 0;
}

static int test_write_then_read(void){
    const uint8_t data[] = { 0x11, 0x22, 0x33 };

    setup();

    CHECK(write_space(SPACE_GENERIC, SPACE_GENERIC_LOW_ADDRESS + 20, data, sizeof(data)) == LCC_OK);
    CHECK(check_write_ok(SPACE_GENERIC, SPACE_GENERIC_LOW_ADDRESS + 20) == 0);

    CHECK(read_space(SPACE_GENERIC, SPACE_GENERIC_LOW_ADDRESS + 20, sizeof(data)) == LCC_OK);
    CHECK(check_read(SPACE_GENERIC, SPACE_GENERIC_LOW_ADDRESS + 20, data, sizeof(data)) == 0);

    return 0;
}

static int test_write_readonly(void){
    const uint8_t data[] = { 0x00 };

    setup();

    CHECK(write_space(SPACE_ACDI_MANUFACTURER, 0, data, sizeof(data)) == LCC_OK);
    CHECK(check_write_rejected(SPACE_ACDI_MANUFACTURER, 0, LCC_ERRCODE_INVALID_ARG) == 0);
    CHECK(acdi_manufacturer_memory[0] == 0x40);

    return 0;
}

static int test_write_missing_space(void){
    const uint8_t data[] = { 0x00 };

    setup();

    CHECK(write_space(SPACE_MISSING, 0, data, sizeof(data)) == LCC_OK);
    CHECK(check_write_rejected(SPACE_MISSING, 0, LCC_ERRCODE_NOT_IMPLEMENTED) == 0);

    // A rejected write must not prevent further requests
    CHECK(write_space(LCC_MEMORY_SPACE_CONFIGURATION_SPACE, 0, data, sizeof(data)) == LCC_OK);
    CHECK(check_write_ok(LCC_MEMORY_SPACE_CONFIGURATION_SPACE, 0) == 0);

    return 0;
}

static int test_write_out_of_range(void){
    const uint8_t data[] = { 0x00, 0x00, 0x00, 0x00 };

    setup();

    // Starts inside of the space, but goes past the end
    CHECK(write_space(LCC_MEMORY_SPACE_CONFIGURATION_SPACE, sizeof(config_memory) - 2, data, sizeof(data)) == LCC_OK);
    CHECK(check_write_rejected(LCC_MEMORY_SPACE_CONFIGURATION_SPACE, sizeof(config_memory) - 2, LCC_ERRCODE_INVALID_ARG) == 0);
    CHECK(config_memory[sizeof(config_memory) - 1] == sizeof(config_memory) - 1);

    return 0;
}

static int test_write_invalid_length(void){
    uint8_t data[65] = { 0 };

    setup();

    CHECK(lcc_remote_memory_write_single_transfer(remote_ctx, memory_node_alias, LCC_MEMORY_SPACE_CONFIGURATION_SPACE, 0, data, 0) == LCC_ERROR_INVALID_ARG);
    CHECK(lcc_remote_memory_write_single_transfer(remote_ctx, memory_node_alias, LCC_MEMORY_SPACE_CONFIGURATION_SPACE, 0, data, 65) == LCC_ERROR_INVALID_ARG);
    CHECK(lcc_remote_memory_write_single_transfer(remote_ctx, memory_node_alias, LCC_MEMORY_SPACE_CONFIGURATION_SPACE, 0, NULL, 1) == LCC_ERROR_INVALID_ARG);

    // An invalid request must not prevent further requests
    CHECK(read_space(LCC_MEMORY_SPACE_CONFIGURATION_SPACE, 0, 4) == LCC_OK);
    CHECK(check_read(LCC_MEMORY_SPACE_CONFIGURATION_SPACE, 0, config_memory, 4) == 0);

    return 0;
}

/*
 * A node may accept a write with 'datagram received OK' without the reply pending
 * bit set, in which case the write is complete and no write reply follows.
 */
static int test_write_no_reply_pending(void){
    const uint8_t data[] = { 0x55 };

    setup();

    CHECK(write_space_to(ack_only_node_alias, LCC_MEMORY_SPACE_CONFIGURATION_SPACE, 3, data, sizeof(data)) == LCC_OK);
    CHECK(check_write_ok(LCC_MEMORY_SPACE_CONFIGURATION_SPACE, 3) == 0);

    // The write is finished, so we can do something else
    CHECK(read_space(LCC_MEMORY_SPACE_CONFIGURATION_SPACE, 0, 4) == LCC_OK);
    CHECK(check_read(LCC_MEMORY_SPACE_CONFIGURATION_SPACE, 0, config_memory, 4) == 0);

    return 0;
}

/*
 * If a node never responds, the request can be cancelled so that other requests can be made
 */
static int request_cancel(void){
    setup();

    memset(&results, 0, sizeof(results));
    CHECK(lcc_remote_memory_read_single_transfer(remote_ctx, silent_node_alias, LCC_MEMORY_SPACE_CONFIGURATION_SPACE, 0, 4) == LCC_OK);
    lcctest_pump_frames();
    CHECK(results.read_count == 0);
    CHECK(results.reject_count == 0);
    CHECK(results.fail_count == 0);
    CHECK(lcc_remote_memory_current_alias_request(remote_ctx) == silent_node_alias);

    // Still waiting for the silent node
    CHECK(lcc_remote_memory_read_single_transfer(remote_ctx, memory_node_alias, LCC_MEMORY_SPACE_CONFIGURATION_SPACE, 0, 4) == LCC_ERROR_MEMORY_TX_IN_PROGRESS);

    CHECK(lcc_remote_memory_cancel_request(remote_ctx) == LCC_OK);
    CHECK(lcc_remote_memory_current_alias_request(remote_ctx) == 0);

    CHECK(read_space(LCC_MEMORY_SPACE_CONFIGURATION_SPACE, 0, 4) == LCC_OK);
    CHECK(check_read(LCC_MEMORY_SPACE_CONFIGURATION_SPACE, 0, config_memory, 4) == 0);

    return 0;
}

/*
 * Cancelling a write means that a late 'datagram received OK' does not complete it
 */
static int request_cancel_write(void){
    const uint8_t data[] = { 0x01 };

    setup();

    memset(&results, 0, sizeof(results));
    CHECK(lcc_remote_memory_write_single_transfer(remote_ctx, silent_node_alias, LCC_MEMORY_SPACE_CONFIGURATION_SPACE, 0, data, sizeof(data)) == LCC_OK);
    lcctest_pump_frames();
    CHECK(results.write_ok_count == 0);
    CHECK(lcc_remote_memory_cancel_request(remote_ctx) == LCC_OK);

    // Another write works, and only it is reported
    CHECK(write_space(LCC_MEMORY_SPACE_CONFIGURATION_SPACE, 5, data, sizeof(data)) == LCC_OK);
    CHECK(check_write_ok(LCC_MEMORY_SPACE_CONFIGURATION_SPACE, 5) == 0);

    return 0;
}

int main(int argc, char** argv){
    if(argc < 2) return 1;

    if(strcmp(argv[1], "info-config-space") == 0){
        return test_info_config_space();
    }else if(strcmp(argv[1], "info-readonly") == 0){
        return test_info_readonly();
    }else if(strcmp(argv[1], "info-low-address") == 0){
        return test_info_low_address();
    }else if(strcmp(argv[1], "info-missing-space") == 0){
        return test_info_missing_space();
    }else if(strcmp(argv[1], "info-cdi") == 0){
        return test_info_cdi();
    }else if(strcmp(argv[1], "read-cdi-null") == 0){
        return test_read_cdi_null();
    }else if(strcmp(argv[1], "read-entire-cdi") == 0){
        return test_read_entire_cdi();
    }else if(strcmp(argv[1], "read-cdi") == 0){
        return test_read_cdi();
    }else if(strcmp(argv[1], "read-config-space") == 0){
        return test_read_config_space();
    }else if(strcmp(argv[1], "read-generic-space") == 0){
        return test_read_generic_space();
    }else if(strcmp(argv[1], "read-generic-low-address") == 0){
        return test_read_generic_low_address();
    }else if(strcmp(argv[1], "read-missing-space") == 0){
        return test_read_missing_space();
    }else if(strcmp(argv[1], "read-out-of-range") == 0){
        return test_read_out_of_range();
    }else if(strcmp(argv[1], "read-entire-space") == 0){
        return test_read_entire_space();
    }else if(strcmp(argv[1], "write-config-space") == 0){
        return test_write_config_space();
    }else if(strcmp(argv[1], "write-generic-space") == 0){
        return test_write_generic_space();
    }else if(strcmp(argv[1], "write-generic-low-address") == 0){
        return test_write_generic_low_address();
    }else if(strcmp(argv[1], "write-max-size") == 0){
        return test_write_max_size();
    }else if(strcmp(argv[1], "write-then-read") == 0){
        return test_write_then_read();
    }else if(strcmp(argv[1], "write-readonly") == 0){
        return test_write_readonly();
    }else if(strcmp(argv[1], "write-missing-space") == 0){
        return test_write_missing_space();
    }else if(strcmp(argv[1], "write-out-of-range") == 0){
        return test_write_out_of_range();
    }else if(strcmp(argv[1], "write-invalid-length") == 0){
        return test_write_invalid_length();
    }else if(strcmp(argv[1], "write-no-reply-pending") == 0){
        return test_write_no_reply_pending();
    }else if(strcmp(argv[1], "request-cancel") == 0){
        return request_cancel();
    }else if(strcmp(argv[1], "request-cancel-write") == 0){
        return request_cancel_write();
    }

    return 1;
}
