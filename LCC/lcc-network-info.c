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
    lcc_node_update node_update_cb;
    struct node_info_list nodes;
};

static struct lcc_node_info* node_info_list_add_node(struct lcc_context* parent_ctx, struct node_info_list* list, uint64_t node_id, int alias){
    if(list->node_list == NULL){
        list->node_list = calloc(10, sizeof(struct lcc_node_info));
        list->size = 10;
    }

    list->node_list[list->len].node_id = node_id;
    list->node_list[list->len].node_alias = alias;
    list->node_list[list->len].parent_ctx = parent_ctx;
    list->len++;
    if(list->len == list->size){
        // Make a new array
        int newSize = list->size * 2;
        struct lcc_node_info* new_array = calloc(newSize, sizeof(struct lcc_node_info));
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
        new_node = node_info_list_add_node(inf->parent, &inf->nodes, node_id, alias);
        if(inf->discovered_cb) inf->discovered_cb(inf, new_node);
    }
}

static void lcc_network_handle_protcol_support_reply(struct lcc_network_info* inf, struct lcc_can_frame* frame){
    int alias = frame->can_id & LCC_NID_ALIAS_MASK;
    struct lcc_node_info* node = NULL;
    int protocol_list_pos = 0;

    for(int x = 0; x < inf->nodes.len; x++){
        if(inf->nodes.node_list[x].node_alias == alias){
            node = inf->nodes.node_list + x;
            break;
        }
    }

    if(node == NULL){
        return;
    }

    // Clear out our protocol list to add in new
    memset(node->protocol_list, 0, sizeof(node->protocol_list));
    if(frame->can_len == 2){
        return;
    }

    if(frame->data[2] & 0x80){
        node->protocol_list[protocol_list_pos++] = LCC_PROTOCOL_SIMPLE;
    }
    if(frame->data[2] & 0x40){
        node->protocol_list[protocol_list_pos++] = LCC_PROTOCOL_DATAGRAM;
    }
    if(frame->data[2] & 0x20){
        node->protocol_list[protocol_list_pos++] = LCC_PROTOCOL_STREAM;
    }
    if(frame->data[2] & 0x10){
        node->protocol_list[protocol_list_pos++] = LCC_PROTOCOL_MEMORY_CONFIGURATION;
    }
    if(frame->data[2] & 0x08){
        node->protocol_list[protocol_list_pos++] = LCC_PROTOCOL_RESERVATION;
    }
    if(frame->data[2] & 0x04){
        node->protocol_list[protocol_list_pos++] = LCC_PROTOCOL_PRODUCER_CONSUMER;
    }
    if(frame->data[2] & 0x02){
        node->protocol_list[protocol_list_pos++] = LCC_PROTOCOL_IDENTIFICATION;
    }
    if(frame->data[2] & 0x01){
        node->protocol_list[protocol_list_pos++] = LCC_PROTOCOL_TEACHING_LEARNING;
    }
    if(frame->can_len == 3){
        goto done;
    }

    if(frame->data[3] & 0x80){
        node->protocol_list[protocol_list_pos++] = LCC_PROTOCOL_REMOTE_BUTTON;
    }
    if(frame->data[3] & 0x40){
        node->protocol_list[protocol_list_pos++] = LCC_PROTOCOL_ABBREVIATED_DEFAULT_CDI;
    }
    if(frame->data[3] & 0x20){
        node->protocol_list[protocol_list_pos++] = LCC_PROTOCOL_DISPLAY;
    }
    if(frame->data[3] & 0x10){
        node->protocol_list[protocol_list_pos++] = LCC_PROTOCOL_SIMPLE_NODE_INFORMATION;
    }
    if(frame->data[3] & 0x08){
        node->protocol_list[protocol_list_pos++] = LCC_PROTOCOL_CONFIGURATION_DESCRIPTION_INFORMATION;
    }
    if(frame->data[3] & 0x04){
        node->protocol_list[protocol_list_pos++] = LCC_PROTOCOL_TRACTION_CONTROL;
    }
    if(frame->data[3] & 0x02){
        node->protocol_list[protocol_list_pos++] = LCC_PROTOCOL_FUNCTION_DESCRIPTION_INFORMATION;
    }
    if(frame->data[3] & 0x01){
        node->protocol_list[protocol_list_pos++] = LCC_PROTOCOL_DCC_COMMAND_STATION;
    }
    if(frame->can_len == 4){
        goto done;
    }

    if(frame->data[4] & 0x80){
        node->protocol_list[protocol_list_pos++] = LCC_PROTOCOL_SIMPLE_TRAIN_NODE;
    }
    if(frame->data[4] & 0x40){
        node->protocol_list[protocol_list_pos++] = LCC_PROTOCOL_FUNCTION_CONFIGURATION;
    }
    if(frame->data[4] & 0x20){
        node->protocol_list[protocol_list_pos++] = LCC_PROTOCOL_FIRMWARE_UPGRADE;
    }
    if(frame->data[4] & 0x10){
        node->protocol_list[protocol_list_pos++] = LCC_PROTOCOL_FIRMWARE_UPGRADE_ACTIVE;
    }

done:
    if(inf->node_update_cb){
        inf->node_update_cb(inf, node);
    }
}

static void lcc_network_handle_simple_node_reply(struct lcc_network_info* inf, struct lcc_can_frame* frame){
    int alias = frame->can_id & LCC_NID_ALIAS_MASK;
    struct lcc_node_info* node = NULL;
    int protocol_list_pos = 0;

    for(int x = 0; x < inf->nodes.len; x++){
        if(inf->nodes.node_list[x].node_alias == alias){
            node = inf->nodes.node_list + x;
            break;
        }
    }

    if(node == NULL){
        return;
    }

    if(frame->can_len < 2){
        return;
    }

    // Assume that data from the node is coming in the correct order
    // (we're not worrying about 'first,' 'middle,' or 'last' at the moment)
    // Perhaps someday we will worry about this more
    for(int x = 0; x < frame->can_len - 2; x++){
        if(node->rx_buffer_location > sizeof(node->rx_buffer)){
            break;
        }
        node->rx_buffer[node->rx_buffer_location++] = frame->data[x + 2];
    }

    if((frame->data[0] & 0x30) == 0x20){
        // This is the last frame - process the data into the simple node information.
        int buffer_pos = 1;
        // The first byte is the number of NULL terminated strings that we are expecting
        int num_strings = node->rx_buffer[0];
        if(num_strings != 4){
            // For now let's only care about the case where there are 4 strings in the first section
            return;
        }

        char* strings[6] = {0};
        int current_string_num = 0;
#define STATE_PARSE_FIRST_STRING_CHAR 0
#define STATE_PARSING_STRING 1
#define STATE_PARSING_INFO_DESC_BYTE 2
        int current_state = STATE_PARSE_FIRST_STRING_CHAR;
        for(int x = 1; x < node->rx_buffer_location; x++){
            if(current_state == STATE_PARSE_FIRST_STRING_CHAR){
                strings[current_string_num++] = node->rx_buffer + x;
                current_state = STATE_PARSING_STRING;
            }
            if(current_state == STATE_PARSING_STRING &&
                    node->rx_buffer[x] == 0){
                if(current_string_num == 4){
                    current_state = STATE_PARSING_INFO_DESC_BYTE;
                    continue;
                }else{
                    current_state = STATE_PARSE_FIRST_STRING_CHAR;
                }
            }
            if(current_state == STATE_PARSING_INFO_DESC_BYTE){
                if(node->rx_buffer[x] != 2){
                    // We can only handle two strings in the node name/description block
                    break;
                }
                current_state = STATE_PARSE_FIRST_STRING_CHAR;
            }
        }

        if(strings[0] != NULL){
            strncpy(node->simple_info.manufacturer_name, strings[0], 40);
        }
        if(strings[1] != NULL){
            strncpy(node->simple_info.model_name, strings[1], 40);
        }
        if(strings[2] != NULL){
            strncpy(node->simple_info.hw_version, strings[2], 20);
        }
        if(strings[3] != NULL){
            strncpy(node->simple_info.sw_version, strings[3], 20);
        }
        if(strings[4] != NULL){
            strncpy(node->simple_info.node_name, strings[4], 62);
        }
        if(strings[5] != NULL){
            strncpy(node->simple_info.node_description, strings[5], 63);
        }

        if(inf->node_update_cb){
            inf->node_update_cb(inf, node);
        }

        node->rx_buffer_location = 0;
    }
}

static void lcc_network_add_event_produced_to_node(struct lcc_network_info* inf, struct lcc_can_frame* frame){
    int alias = frame->can_id & LCC_NID_ALIAS_MASK;
    struct lcc_node_info* node = NULL;
    int protocol_list_pos = 0;

    for(int x = 0; x < inf->nodes.len; x++){
        if(inf->nodes.node_list[x].node_alias == alias){
            node = inf->nodes.node_list + x;
            break;
        }
    }

    if(node == NULL){
        return;
    }

    if(frame->can_len < 8){
        return;
    }

    uint64_t event_id = lcc_get_eventid_from_data(frame);
    event_list_add_event(&node->produced_events, event_id);

    if(inf->node_update_cb){
        inf->node_update_cb(inf, node);
    }
}

static void lcc_network_add_event_consumed_to_node(struct lcc_network_info* inf, struct lcc_can_frame* frame){
    int alias = frame->can_id & LCC_NID_ALIAS_MASK;
    struct lcc_node_info* node = NULL;
    int protocol_list_pos = 0;

    for(int x = 0; x < inf->nodes.len; x++){
        if(inf->nodes.node_list[x].node_alias == alias){
            node = inf->nodes.node_list + x;
            break;
        }
    }

    if(node == NULL){
        return;
    }

    if(frame->can_len < 8){
        return;
    }

    uint64_t event_id = lcc_get_eventid_from_data(frame);
    event_list_add_event(&node->consumed_events, event_id);

    if(inf->node_update_cb){
        inf->node_update_cb(inf, node);
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
    }else if(mti == LCC_MTI_PROTOCOL_SUPPORT_REPLY){
        lcc_network_handle_protcol_support_reply(ctx, frame);
    }else if(mti == LCC_MTI_SIMPLE_NODE_INFORMATION_REPLY){
        lcc_network_handle_simple_node_reply(ctx, frame);
    }else if(mti == LCC_MTI_CONSUMER_IDENTIFIED_VALID ||
             mti == LCC_MTI_CONSUMER_IDENTIFIED_INVALID ||
             mti == LCC_MTI_CONSUMER_IDENTIFIED_UNKNOWN){
        lcc_network_add_event_consumed_to_node(ctx, frame);
    }else if(mti == LCC_MTI_PRODUCER_IDENTIFIED_VALID ||
             mti == LCC_MTI_PRODUCER_IDENTIFIED_INVALID ||
             mti == LCC_MTI_PRODUCER_IDENTIFIED_UNKNOWN){
        lcc_network_add_event_produced_to_node(ctx, frame);
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
    return inf->parent->write_function(inf->parent, &frame);
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

int lcc_network_set_node_changed_callback(struct lcc_network_info* inf, lcc_node_update fn){
    if(!inf) return LCC_ERROR_INVALID_ARG;
    inf->node_update_cb = fn;
    return LCC_OK;
}
