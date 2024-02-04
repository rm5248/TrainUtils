/* SPDX-License-Identifier: GPL-2.0 */
#ifndef LCC_SIMPLENODEINFO_H
#define LCC_SIMPLENODEINFO_H

#ifdef __cplusplus
extern "C" {
#endif

struct lcc_simple_node_info;

/**
 * Get the manufacuterer name from the LCC node info.
 *
 * @param inf
 * @return Manufacturer name, or NULL
 */
const char* lcc_simple_node_info_manufacturer_name( struct lcc_simple_node_info* inf );

/**
 * Get the model name from the LCC node info.
 *
 * @param inf
 * @return Model name, or NULL
 */
const char* lcc_simple_node_info_model_name( struct lcc_simple_node_info* inf );

const char* lcc_simple_node_info_hw_version( struct lcc_simple_node_info* inf );

const char* lcc_simple_node_info_sw_version( struct lcc_simple_node_info* inf );

const char* lcc_simple_node_info_node_name( struct lcc_simple_node_info* inf );

const char* lcc_simple_node_info_node_description( struct lcc_simple_node_info* inf );

#ifdef __cplusplus
} /* extern C */
#endif

#endif // LCC_SIMPLENODEINFO_H
