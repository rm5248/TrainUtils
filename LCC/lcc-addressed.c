/* SPDX-License-Identifier: GPL-2.0 */
#include <string.h>
#include <stdio.h>

#include "lcc-addressed.h"
#include "lcc-common-internal.h"
#include "lcc-datagram.h"

int lcc_handle_addressed(struct lcc_context* ctx, struct lcc_can_frame* frame){
    uint16_t alias = (frame->data[0] << 8) | frame->data[1];
    if(alias != ctx->node_alias){
        // This is not meant for us
        return LCC_OK;
    }

    uint16_t mti = (frame->can_id & LCC_VARIABLE_FIELD_MASK) >> 12;
    uint16_t sender = (frame->can_id & LCC_NID_ALIAS_MASK);
    struct lcc_can_frame ret_frame;
    memset(&ret_frame, 0, sizeof(ret_frame));

    if(mti == LCC_MTI_BASIC_VERIFY_NODE_ID_NUM_ADDRESSED){
        // Respond with Node ID
        lcc_set_lcb_variable_field(&ret_frame, ctx, LCC_MTI_BASIC_VERIFIED_NODE_ID_NUM | LCC_MTI_SIMPLE);
        lcc_set_lcb_can_frame_type(&ret_frame, 1);
        lcc_set_nodeid_in_data(&ret_frame, ctx->unique_id);
        return ctx->write_function(ctx, &ret_frame);
    }else if(mti == LCC_MTI_PROTOCOL_SUPPORT_INQUIRE){
        // Respond with protocol support reply
        // Note: JMRI does not like less than 8 bytes here, even though technically it is in-spec
        // See: https://github.com/openlcb/OpenLCB_Java/issues/210
        ret_frame.can_len = 8;
        ret_frame.data[0] = (sender & 0xFF00) >> 8;
        ret_frame.data[1] = (sender & 0x00FF);
        ret_frame.data[2] = 0x80 /* Simple protocol */
                | 0x04 /* producer/consumer protocol */
                | 0x02 /* identification protocol */;
        if(ctx->datagram_context){
            ret_frame.data[2] |= 0x40; /* Datagram protocol */
        }
        ret_frame.data[3] = 0x10 /* Simple node information */;
        if(ctx->memory_context){
            // TODO this should maybe be a bit smarter(e.g. pass in CDI as configuration
            // option to the lcc_context), but this suffices for now.
            ret_frame.data[3] |= 0x08; /* CDI protocol */
        }
        ret_frame.data[4] = 0;
        ret_frame.data[5] = 0;
        ret_frame.data[6] = 0;
        ret_frame.data[7] = 0;
        lcc_set_lcb_variable_field(&ret_frame, ctx, LCC_MTI_PROTOCOL_SUPPORT_REPLY | LCC_MTI_ADDRESSED);
        lcc_set_lcb_can_frame_type(&ret_frame, 1);
        return ctx->write_function(ctx, &ret_frame);
    }else if(mti == LCC_MTI_SIMPLE_NODE_INFORMATION_REQUEST){
        // Respond with our simple information
        char max_simple_data[sizeof(struct lcc_simple_node_info) + 2];
        int manufacturer_string_len = strlen(ctx->simple_info.manufacturer_name);
        int model_string_len = strlen(ctx->simple_info.model_name);
        int hardware_string_len = strlen(ctx->simple_info.hw_version);
        int software_string_len = strlen(ctx->simple_info.sw_version);
        int node_name_len = strlen(ctx->simple_info.node_name);
        int node_description_len = strlen(ctx->simple_info.node_description);
        int max_simple_data_len = 0;

        max_simple_data[0] = 4;
        max_simple_data_len++;

        // Copy manufacturer string
        if(manufacturer_string_len == 0){
            max_simple_data[max_simple_data_len] = 0;
            max_simple_data_len++;
        }else{
            memcpy(&max_simple_data[max_simple_data_len], ctx->simple_info.manufacturer_name, manufacturer_string_len + 1);
            max_simple_data_len += manufacturer_string_len + 1;
        }

        // Copy model string
        if(model_string_len == 0){
            max_simple_data[max_simple_data_len] = 0;
            max_simple_data_len++;
        }else{
            memcpy(&max_simple_data[max_simple_data_len], ctx->simple_info.model_name, model_string_len + 1);
            max_simple_data_len += model_string_len + 1;
        }

        // Copy harware version string
        if(hardware_string_len == 0){
            max_simple_data[max_simple_data_len] = 0;
            max_simple_data_len++;
        }else{
            memcpy(&max_simple_data[max_simple_data_len], ctx->simple_info.hw_version, hardware_string_len + 1);
            max_simple_data_len += hardware_string_len + 1;
        }

        // Copy software version string
        if(software_string_len == 0){
            max_simple_data[max_simple_data_len] = 0;
            max_simple_data_len++;
        }else{
            memcpy(&max_simple_data[max_simple_data_len], ctx->simple_info.sw_version, software_string_len + 1);
            max_simple_data_len += software_string_len + 1;
        }

        max_simple_data[max_simple_data_len] = 2;
        max_simple_data_len++;
        // Copy node name
        if(node_name_len == 0){
            max_simple_data[max_simple_data_len] = 0;
            max_simple_data_len++;
        }else{
            memcpy(&max_simple_data[max_simple_data_len], ctx->simple_info.node_name, node_name_len + 1);
            max_simple_data_len += node_name_len + 1;
        }

        // Copy node description
        if(node_description_len == 0){
            max_simple_data[max_simple_data_len] = 0;
            max_simple_data_len++;
        }else{
            memcpy(&max_simple_data[max_simple_data_len], ctx->simple_info.node_description, node_description_len + 1);
            max_simple_data_len += node_description_len + 1;
        }

        // Now that we have all of our information, send it all out!
        int data_offset = 0;
        for(int numPackets = 0; numPackets < max_simple_data_len / 6 + 1; numPackets++){
            int numBytesToCopy = 6;
            lcc_set_lcb_variable_field(&ret_frame, ctx, LCC_MTI_SIMPLE_NODE_INFORMATION_REPLY);
            lcc_set_lcb_can_frame_type(&ret_frame, 1);
            if(numPackets == 0){
                lcc_set_flags_and_dest_alias(&ret_frame, LCC_FLAG_FRAME_FIRST, sender);
            }else if(numPackets == (max_simple_data_len / 6)){
                numBytesToCopy = max_simple_data_len % 6;
                lcc_set_flags_and_dest_alias(&ret_frame, LCC_FLAG_FRAME_LAST, sender);
            }else{
                lcc_set_flags_and_dest_alias(&ret_frame, LCC_FLAG_FRAME_MIDDLE, sender);
            }

            memcpy(&ret_frame.data[2], max_simple_data + data_offset, numBytesToCopy);
            ret_frame.can_len = 2 + numBytesToCopy;
            int write_ret = ctx->write_function(ctx, &ret_frame);
            if(write_ret != LCC_OK){
                return write_ret;
            }

            data_offset += 6;
        }
    }else if(mti == LCC_MTI_EVENT_IDENTIFY){
        return lcc_send_events_produced(ctx);
    }

    return LCC_OK;
}
