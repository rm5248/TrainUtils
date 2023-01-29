/* SPDX-License-Identifier: GPL-2.0 */
#ifndef LCC_NODE_INFO_H
#define LCC_NODE_INFO_H
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct lcc_node_info;
struct lcc_simple_node_info;

enum lcc_protocols{
    LCC_PROTOCOL_INVALID = 0,
    LCC_PROTOCOL_SIMPLE,
    LCC_PROTOCOL_DATAGRAM,
    LCC_PROTOCOL_STREAM,
    LCC_PROTOCOL_MEMORY_CONFIGURATION,
    LCC_PROTOCOL_RESERVATION,
    LCC_PROTOCOL_PRODUCER_CONSUMER,
    LCC_PROTOCOL_IDENTIFICATION,
    LCC_PROTOCOL_TEACHING_LEARNING,
    LCC_PROTOCOL_REMOTE_BUTTON,
    LCC_PROTOCOL_ABBREVIATED_DEFAULT_CDI,
    LCC_PROTOCOL_DISPLAY,
    LCC_PROTOCOL_SIMPLE_NODE_INFORMATION,
    LCC_PROTOCOL_CONFIGURATION_DESCRIPTION_INFORMATION,
    LCC_PROTOCOL_TRACTION_CONTROL,
    LCC_PROTOCOL_FUNCTION_DESCRIPTION_INFORMATION,
    LCC_PROTOCOL_DCC_COMMAND_STATION,
    LCC_PROTOCOL_SIMPLE_TRAIN_NODE,
    LCC_PROTOCOL_FUNCTION_CONFIGURATION,
    LCC_PROTOCOL_FIRMWARE_UPGRADE,
    LCC_PROTOCOL_FIRMWARE_UPGRADE_ACTIVE,
};

uint64_t lcc_node_info_get_id(struct lcc_node_info* inf);

int lcc_node_info_get_alias(struct lcc_node_info* inf);

/**
 * Get a list of all events that this node produces
 *
 * @param inf
 * @param produced_list A pointer that will be filled in to point at the start of the list of events produced
 * @param produced_len The length of the produced_list
 * @return
 */
int lcc_node_info_get_events_produced(struct lcc_node_info* inf, uint64_t** produced_list, int* produced_len);

/**
 * Get a list of all events that this node consumes
 *
 * @param inf
 * @param consumed_list A pointer that will be filled in to point at the start of the list of events consumed
 * @param consumed_len The length of the consumed_list
 * @return
 */
int lcc_node_info_get_events_consumed(struct lcc_node_info* inf, uint64_t** consumed_list, int* consumed_len);

/**
 * Get the 'simple' node information for this node.
 *
 * @param inf
 * @return
 */
struct lcc_simple_node_info* lcc_node_info_get_simple(struct lcc_node_info* inf);

/**
 * Get a list of protocols supported by this node.
 *
 * @param inf
 * @param protocols_list
 * @param protocols_len
 * @return
 */
int lcc_node_info_get_protocols_supported(struct lcc_node_info* inf, enum lcc_protocols** protocols_list, int* protocols_len);

/**
 * Refresh the simple node information of this node.
 *
 * @param inf
 * @return
 */
int lcc_node_refresh_simple_info(struct lcc_node_info* inf);

int lcc_node_refresh_events_produced(struct lcc_node_info* inf);

int lcc_node_refresh_events_consumed(struct lcc_node_info* inf);

int lcc_node_refresh_protocol_support(struct lcc_node_info* inf);

#ifdef __cplusplus
} /* extern C */
#endif

#endif /* LCC_NETWORK_INFO_H */
