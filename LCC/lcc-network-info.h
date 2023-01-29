/* SPDX-License-Identifier: GPL-2.0 */
#ifndef LCC_NETWORK_INFO_H
#define LCC_NETWORK_INFO_H

#ifdef __cplusplus
extern "C" {
#endif

struct lcc_network_info;
struct lcc_context;
struct lcc_can_frame;
struct lcc_node_info;

/**
 * A callback that will be called when a new node is seen on the network
 */
typedef void(*lcc_discovered_new_node)(struct lcc_network_info* inf, struct lcc_node_info* new_node);

/**
 * A callback that will be called when information in the node is updated(e.g. events produced/consumed,
 * simple node information, etc.)
 */
typedef void(*lcc_node_update)(struct lcc_network_info* inf, struct lcc_node_info* node);

struct lcc_network_info* lcc_network_new(struct lcc_context* ctx);

void lcc_network_free(struct lcc_network_info* inf);

/**
 * Call this method with a CAN frame when data comes in over the CAN bus.
 *
 * @param frame The incoming frame
 * @return LCC status int
 */
int lcc_network_incoming_frame(struct lcc_network_info* inf, struct lcc_can_frame* frame);

/**
 * Get a list of all nodes that we have seen on the network.
 *
 * @param inf The network info to get the node list for
 * @param node_list A pointer to the start of an array of pointers that will be filled in
 * @param node_list_len A pointer that will be filled in with the number of nodes in the list
 * @return The number of entries filled out in the node_list param, or negative error code
 */
int lcc_network_get_node_list(struct lcc_network_info* inf, struct lcc_node_info** node_list, int node_list_len);

/**
 * Refresh/re-build our network tree by sending a 'Verify Node ID Number Global' message.
 * Note that this will clear out the internal node list, such that it will only contain nodes that actually exist
 * and have responded.
 *
 * @param inf
 * @return
 */
int lcc_network_refresh_nodes(struct lcc_network_info* inf);

/**
 * Set a callback that will be called when a new node is seen on the network.
 *
 * @param inf
 * @param fn
 * @return
 */
int lcc_network_set_new_node_callback(struct lcc_network_info* inf, lcc_discovered_new_node fn);

int lcc_network_set_node_changed_callback(struct lcc_network_info* inf, lcc_node_update fn);

int lcc_network_set_userdata(struct lcc_network_info* inf, void* user_data);

void* lcc_network_get_userdata(struct lcc_network_info* inf);

#ifdef __cplusplus
} /* extern C */
#endif

#endif /* LCC_NETWORK_INFO_H */
