/* SPDX-License-Identifier: GPL-2.0 */
#include "lcc-common-internal.h"
#include "lcc-simple-node-info.h"

#include <stdlib.h>

#ifdef LCC_SIMPLE_NODE_INFO_SMALL
static const char* find_null_terminated_string_x( struct lcc_simple_node_info* inf, int x ){
    int current_string = 0;
    const char* current_string_start = inf->node_information;

    for(unsigned int i = 0; i < sizeof(struct lcc_simple_node_info); i++ ){
        if(inf->node_information[i] == 0 && current_string == x){
            return current_string_start;
        }else if(inf->node_information[i] == 0){
            current_string++;
            current_string_start = &inf->node_information[ i + 1 ];
        }
    }

    return NULL;
}
#endif

const char* lcc_simple_node_info_manufacturer_name( struct lcc_simple_node_info* inf ){
    if(inf == NULL){
        return NULL;
    }

#if LCC_SIMPLE_NODE_INFO_SMALL
    return find_null_terminated_string_x( inf, 0 );
#else
    return inf->manufacturer_name;
#endif
}

const char* lcc_simple_node_info_model_name( struct lcc_simple_node_info* inf ){
    if(inf == NULL){
        return NULL;
    }

#if LCC_SIMPLE_NODE_INFO_SMALL
    return find_null_terminated_string_x( inf, 1 );
#else
    return inf->model_name;
#endif
}

const char* lcc_simple_node_info_hw_version( struct lcc_simple_node_info* inf ){
    if(inf == NULL){
        return NULL;
    }

#if LCC_SIMPLE_NODE_INFO_SMALL
    return find_null_terminated_string_x( inf, 2 );
#else
    return inf->hw_version;
#endif
}

const char* lcc_simple_node_info_sw_version( struct lcc_simple_node_info* inf ){
    if(inf == NULL){
        return NULL;
    }

#if LCC_SIMPLE_NODE_INFO_SMALL
    return find_null_terminated_string_x( inf, 3 );
#else
    return inf->sw_version;
#endif
}

const char* lcc_simple_node_info_node_name( struct lcc_simple_node_info* inf ){
    if(inf == NULL){
        return NULL;
    }

#if LCC_SIMPLE_NODE_INFO_SMALL
    return find_null_terminated_string_x( inf, 4 );
#else
    return inf->node_name;
#endif
}

const char* lcc_simple_node_info_node_description( struct lcc_simple_node_info* inf ){
    if(inf == NULL){
        return NULL;
    }

#if LCC_SIMPLE_NODE_INFO_SMALL
    return find_null_terminated_string_x( inf, 5 );
#else
    return inf->node_description;
#endif
}
