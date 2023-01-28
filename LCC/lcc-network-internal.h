/* SPDX-License-Identifier: GPL-2.0 */
#ifndef LCC_NETWORK_INT_H
#define LCC_NETWORK_INT_H

#ifndef LIBLCC_BUILD
#error "Internal header, do not use in client code!"
#endif

#include <stdint.h>
#include "lcc-common.h"
#include "lcc-common-internal.h"
#include "lcc-node-info.h"

struct lcc_node_info{
    uint64_t node_id;
    int node_alias;
    struct event_list produced_events;
    struct event_list consumed_events;
    enum lcc_protocols protocol_list[20];
    struct lcc_simple_node_info simple_info;
    struct lcc_context* parent_ctx;
};

struct lcc_node_info* lcc_node_info_new(struct lcc_context* parent);

void lcc_node_info_free(struct lcc_node_info* inf);

#endif /* LCC_NETWORK_INT_H */
