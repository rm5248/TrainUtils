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
};

static struct remote_results results;
static struct lcc_remote_memory_context* remote_ctx;
static uint16_t memory_node_alias;

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

static void setup(void){
    struct lcc_context** ctx = lcctest_create_contexts(2);

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
    lcc_memory_map_new(memory_ctx, segments);
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
    }

    return 1;
}
