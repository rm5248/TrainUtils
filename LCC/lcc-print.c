/* SPDX-License-Identifier: GPL-2.0 */
#include "lcc-print.h"

static const char* decode_mti2(int can_frame_type, uint16_t mti){
    if( can_frame_type >= 2 &&
            can_frame_type <= 7 ){
        return "Addressed packet";
    }

    switch(mti){
    }

    return "unknown MTI";
}

static void decode_mti(FILE* output, int can_frame_type, uint16_t mti){
    if( can_frame_type >= 2 &&
            can_frame_type <= 7 ){
        // This is not an MTI
        return;
    }

    if(mti & (0x01 << 13)){
        fprintf( output, "    Special bit\n");
    }
    if(mti & (0x01 << 12)){
        fprintf( output, "    Stream or datagram\n");
    }
    fprintf( output, "    Priority: %d\n", (mti & LCC_MTI_PRIORITY_MASK) >> 10 );
    fprintf( output, "    Type within priority: %d\n", (mti & LCC_MTI_TYPE_WITHIN_PRIORITY_MASK) >> 5 );
    if(mti & (0x01 << 4)){
        fprintf( output, "    Simple protocol\n");
    }
    if(mti & (0x01 << 3)){
        fprintf( output, "    Address present\n");
    }
    if(mti & (0x01 << 2)){
        fprintf( output, "    Event num present\n");
    }
    fprintf( output, "    Modifier within priority/type: %d\n", mti & 0x3);
}

static const char* decode_can_frame_type(int can_frame_type){
    switch( can_frame_type ){
    case 0:
        return "reserved(0)";
    case 1:
        return "Global OR addressed";
    case 2:
        return "Datagram only";
    case 3:
        return "Datagram first";
    case 4:
        return "Datagram middle";
    case 5:
        return "Datagram last";
    case 6:
        return "reserved(6)";
    case 7:
        return "stream";
    }

    return "unknown can frame type";
}

void lcc_decode_frame(struct lcc_can_frame* frame, FILE* output, int print_flags){
    if( frame == NULL || output == NULL ){
        return;
    }
    int x = 0;

    int can_frame_type = (frame->can_id & LCC_CAN_FRAME_TYPE_MASK) >> 24;
    uint16_t mti = (frame->can_id & LCC_VARIABLE_FIELD_MASK) >> 12;
    fprintf( output, "Header: 0x%08X Data length: %d\n", frame->can_id, frame->can_len );
    fprintf( output, "  CAN Frame type: %d(%s)\n", can_frame_type, decode_can_frame_type(can_frame_type));
    fprintf( output, "  Variable field: 0x%03X\n", mti );
    decode_mti( output, can_frame_type, mti );
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
