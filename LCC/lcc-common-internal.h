/* SPDX-License-Identifier: GPL-2.0 */
#ifndef LIBLCC_COMMON_INT_H
#define LIBLCC_COMMON_INT_H

#ifndef LIBLCC_BUILD
#error "Internal header, do not use in client code!"
#endif

#include <stdint.h>

#include "lcc-common.h"

struct lcc_context{
    uint64_t unique_id;
    union{
        int16_t flags;
        int16_t reserved : 13, node_alias_state : 2, state : 1;
    };
    int16_t node_alias;
    lcc_write_fn write_function;
    void* user_data;

    // Simple node information
    char manufacturer_name[41];
    char model_name[41];
    char hw_version[21];
    char sw_version[21];
    char node_name[63];
    char node_description[64];
};

#define LCC_FLAG_FRAME_ONLY 0
#define LCC_FLAG_FRAME_FIRST 1
#define LCC_FLAG_FRAME_LAST 2
#define LCC_FLAG_FRAME_MIDDLE 3

void lcc_set_lcb_variable_field(struct lcc_can_frame* frame, struct lcc_context* ctx, int variable_field);

void lcc_set_lcb_can_frame_type(struct lcc_can_frame* frame, int type);

void lcc_set_nodeid_in_data(struct lcc_can_frame* frame, uint64_t node_id);

uint64_t lcc_get_node_id_from_data(struct lcc_can_frame* frame);

void lcc_set_flags_and_dest_alias(struct lcc_can_frame* frame, int flag_frame, int alias);

#endif
