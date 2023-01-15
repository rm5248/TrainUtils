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
};

#endif
