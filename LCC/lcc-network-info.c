/* SPDX-License-Identifier: GPL-2.0 */
#include <string.h>
#include <stdlib.h>
#include "lcc-network-info.h"
#include "lcc-common-internal.h"
#include "lcc.h"
#include "lcc-network-internal.h"

struct node_info_list{
    struct lcc_node_info* node_list;
    int size;
    int len;
};

struct lcc_network_info{
    struct lcc_context* parent;
    void* user_data;
    lcc_discovered_new_node discovered_cb;
    struct node_info_list nodes;
};

static struct lcc_node_info* node_info_list_add_node(struct node_info_list* list, uint64_t node_id, int alias){
    if(list->node_list == NULL){
        list->node_list = malloc(sizeof(struct lcc_node_info) * 10);
        list->size = 10;
    }

    list->node_list[list->len].node_id = node_id;
    list->node_list[list->len].node_alias = alias;
    list->len++;
    if(list->len == list->size){
        // Make a new array
        int newSize = list->size * 2;
        struct lcc_node_info* new_array = malloc(sizeof(struct lcc_node_info) * newSize);
        memcpy(new_array, list->node_list, sizeof(struct lcc_node_info) * list->size);
        free(list->node_list);
        list->node_list = new_array;
        list->size = newSize;
    }

    return &list->node_list[list->len - 1];
}

static void lcc_network_add_node_if_new(struct lcc_network_info* inf, struct lcc_can_frame* frame){
    int alias = (frame->can_id & LCC_NID_ALIAS_MASK);
    uint64_t node_id = lcc_get_node_id_from_data(frame);
    struct lcc_node_info* new_node = NULL;

    // Search thru our list, add in the node if it does not exist.
    for(int x = 0; x < inf->nodes.len; x++){
        if(inf->nodes.node_list[x].node_id == node_id){
            new_node = &(inf->nodes.node_list[x]);
            break;
        }
    }

    if(!new_node){
        new_node = node_info_list_add_node(&inf->nodes, node_id, alias);
        if(inf->discovered_cb) inf->discovered_cb(inf, new_node);
    }
}

struct lcc_network_info* lcc_network_new(struct lcc_context* ctx){
    if(!ctx) return NULL;
    struct lcc_network_info* inf = malloc(sizeof(struct lcc_network_info));
    if(!inf) return NULL;

    memset(inf, 0, sizeof(struct lcc_network_info));
    inf->parent = ctx;

    return inf;
}

void lcc_network_free(struct lcc_network_info* inf){
    if(!inf) return;
    if(inf->nodes.node_list) free(inf->nodes.node_list);
    free(inf);
}

int lcc_network_incoming_frame(struct lcc_network_info* ctx, struct lcc_can_frame* frame){
    if(ctx == NULL || frame == NULL){
        return LCC_ERROR_INVALID_ARG;
    }

    uint16_t mti = (frame->can_id & LCC_VARIABLE_FIELD_MASK) >> 12;

    if(mti == LCC_MTI_BASIC_VERIFIED_NODE_ID_NUM){
        lcc_network_add_node_if_new(ctx, frame);
    }

    return LCC_OK;
}

int lcc_network_get_node_list(struct lcc_network_info* inf, struct lcc_node_info** node_list, int node_list_len){
    if(!inf || !node_list || !node_list_len) return LCC_ERROR_INVALID_ARG;

    int x;
    for(x = 0; x < inf->nodes.len; x++){
        node_list[x] = &(inf->nodes.node_list[x]);
        if(x > node_list_len){
            break;
        }
    }

    return x;
}

int lcc_network_refresh_nodes(struct lcc_network_info* inf){
    if(!inf) return LCC_ERROR_INVALID_ARG;

    // Clear our list of nodes
    free(inf->nodes.node_list);
    inf->nodes.node_list = NULL;
    inf->nodes.len = 0;
    inf->nodes.size = 0;

    struct lcc_can_frame frame;
    memset(&frame, 0, sizeof(frame));

    lcc_set_lcb_variable_field(&frame, inf->parent, LCC_MTI_BASIC_VERIFY_NODE_ID_NUM_GLOBAL);
    lcc_set_lcb_can_frame_type(&frame, 1);
    inf->parent->write_function(inf->parent, &frame);

    return LCC_OK;
}

int lcc_network_set_new_node_callback(struct lcc_network_info* inf, lcc_discovered_new_node fn){
    if(!inf) return LCC_ERROR_INVALID_ARG;
    inf->discovered_cb = fn;
    return LCC_OK;
}

int lcc_network_set_userdata(struct lcc_network_info* inf, void* user_data){
    if(!inf) return LCC_ERROR_INVALID_ARG;
    inf->user_data = user_data;
    return LCC_OK;
}

void* lcc_network_get_userdata(struct lcc_network_info* inf){
    if(!inf) return NULL;
    return inf->user_data;
}
