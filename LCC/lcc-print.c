/* SPDX-License-Identifier: GPL-2.0 */
#include "lcc-print.h"

void lcc_decode_frame(struct lcc_can_frame* frame, FILE* output, int print_flags){
    if( frame == NULL || output == NULL ){
        return;
    }
    int x = 0;

    fprintf( output, "Header: 0x%08X Data length: %d\n", frame->can_id, frame->can_len );
    fprintf( output, "  CAN Frame type: %d\n", (frame->can_id & LCC_CAN_FRAME_TYPE_MASK) >> 24);
    fprintf( output, "  Variable field: 0x%03X\n", (frame->can_id & LCC_VARIABLE_FIELD_MASK) >> 12 );
    fprintf( output, "  Alias: 0x%03X\n", frame->can_id & LCC_NID_ALIAS_MASK );
    fprintf( output, "  Data: " );
    for( x = 0; x < frame->can_len; x++ ){
        fprintf( output, "0x%02X ", frame->data[x] );
    }
    if( x == 0 ){
        fprintf( output, "  (no data)\n" );
    }else{
        fprintf( output, "\n" );
    }
}
