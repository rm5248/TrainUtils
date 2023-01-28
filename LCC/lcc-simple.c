/* SPDX-License-Identifier: GPL-2.0 */
#include <stdlib.h>

#include "lcc-simple.h"
#include "lcc-common-internal.h"

int lcc_handle_simple_protocol(struct lcc_context* ctx, struct lcc_can_frame* frame){
    if(ctx == NULL || frame == NULL){
        return LCC_ERROR_INVALID_ARG;
    }

    uint16_t mti = (frame->can_id & LCC_VARIABLE_FIELD_MASK) >> 12;

    if((mti & LCC_MTI_SIMPLE) == 0){
        // This is not a simple frame
        return LCC_ERROR_GENERIC;
    }

    if(mti == LCC_MTI_BASIC_VERIFY_NODE_ID_NUM_GLOBAL){
        // Check to see if this contains a node ID.  If it does and it doesn't match,
        // don't send anything back
        if(frame->can_len == 6){
            uint64_t addressed_data = lcc_get_node_id_from_data(frame);
            if(addressed_data != ctx->unique_id){
                return LCC_OK;
            }
        }

        // Respond with frame ID
        struct lcc_can_frame frame;
        memset(&frame, 0, sizeof(frame));
        lcc_set_lcb_variable_field(&frame, ctx, LCC_MTI_BASIC_VERIFIED_NODE_ID_NUM | LCC_MTI_SIMPLE);
        lcc_set_lcb_can_frame_type(&frame, 1);
        lcc_set_nodeid_in_data(&frame, ctx->unique_id);
        ctx->write_function(ctx, &frame);
        return LCC_OK;
    }

    return LCC_OK;
}
