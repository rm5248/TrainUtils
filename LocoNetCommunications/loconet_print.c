#include <stdio.h>

#include "loconet_print.h"

void loconet_print_directions_and_func( FILE* output,  uint8_t byte ){
	fprintf( output, "  Direction: %s\n", byte & 0x20 ? "REV" : "FWD"  );
	fprintf( output, "  Functions: " );
	//F0 is in the upper byte; the lower byte contains F1-F4
	if( byte & 0x10 ) fprintf( output, "F0 " );
	if( byte & 0x01 ) fprintf( output, "F1 " );
	if( byte & 0x02 ) fprintf( output, "F2 " );
	if( byte & 0x04 ) fprintf( output, "F3 " );
	if( byte & 0x08 ) fprintf( output, "F4 " );
	fprintf( output, "\n" );
}

void loconet_print_slot_status( FILE* output, uint8_t stat ){
	uint8_t decoderType = stat & 0x07;
	uint8_t slotStatus = ( stat & ( 0x03 << 4 ) ) >> 4;
	uint8_t consistStatus = ( stat & ( 0x03 << 6 ) ) >> 6;

	if( decoderType == 0x00 ){
		fprintf( output, "  28 step/ 3byte packet regular mode\n" );
	}else if( decoderType == 0x01 ){
		fprintf( output, "  28 step/ trinary packets\n" );
	}else if( decoderType == 0x02 ){
		fprintf( output, "  14 step mode\n" );
	}else if( decoderType == 0x03 ){
		fprintf( output, "  128 mode packets\n" );
	}else if( decoderType == 0x04 ){
		fprintf( output, "  28 step, advanced DCC consisting\n" );
	}else if( decoderType == 0x07 ){
		fprintf( output, "  128 step, advanced DCC consisting\n" );
	}else{
		fprintf( output, "  Unkown DCC decoder type\n" );
	}

	fprintf( output, "  Slot status: " );
	if( slotStatus == 0x00 ){
		fprintf( output, "FREE\n" );
	}else if( slotStatus == 0x01 ){
		fprintf( output, "COMMON\n" );
	}else if( slotStatus == 0x02 ){
		fprintf( output, "IDLE\n" );
	}else if( slotStatus == 0x03 ){
		fprintf( output, "IN_USE\n" );
	}

	fprintf( output, "  Consist status: " );
	if( consistStatus == 0x00 ){
		fprintf( output, "FREE, no consist\n" );
	}else if( consistStatus == 0x01 ){
		fprintf( output, "logical consist sub-member\n" );
	}else if( consistStatus == 0x02 ){
		fprintf( output, "logical consist top\n" );
	}else if( consistStatus == 0x03 ){
		fprintf( output, "logical mid consist\n" );
	}
}

void loconet_print_track_status( FILE* output, uint8_t trk ){
	if( trk & 0x01 ){
		fprintf( output, "  DCC Power ON, Global power UP\n" );
	}else{
		fprintf( output, "  DCC Power OFF\n" );
	}

	if( !(trk & 0x02) ){
		fprintf( output, "  Track PAUSED, Broadcast ESTOP\n" );
	}

	if( trk & 0x04 ){
		fprintf( output, "  Master has LocoNet 1.1\n" );
	}else{
		fprintf( output, "  Master is DT200\n" );
	}

	if( trk & 0x08 ){
		fprintf( output, "  Programming Track Busy\n" );
	}
}

void loconet_print_message( FILE* output, const struct loconet_message* message ){
	int x;
    fprintf( output, "Loconet message[" );
	switch( message->opcode ){
		case LN_OPC_LOCO_SPEED:
			fprintf( output, "Locomotive speed message \n ");
			fprintf( output, "  Slot: %d\n", message->speed.slot );
			fprintf( output, "  Speed: %d\n", message->speed.speed );
			break;
		case LN_OPC_LOCO_DIR_FUNC:
			fprintf( output, "Locomotive direction/functions message: \n" );
            fprintf( output, "  Slot: %d\n", message->direction_functions.slot );
            loconet_print_directions_and_func( output, message->direction_functions.dir_funcs );
			fprintf( output, "\n" );
			break;
		case LN_OPC_LONG_ACK:
			fprintf( output, "Long ACK \n" );
			fprintf( output, "  Long Opcode: 0x%X\n", message->ack.lopc );
			fprintf( output, "  Status: 0x%X\n", message->ack.ack );
			if( message->ack.ack == 0 ) fprintf( output, "  STATUS FAIL\n" );
			break;
		case LN_OPC_LOCO_ADDR:
			fprintf( output, "Request locomotive addr\n" );
			fprintf( output, "  Address: %d\n", message->addr.locoAddrHi << 7 | message->addr.locoAddrLo );
			break;
		case LN_OPC_SLOT_READ_DATA:
		case LN_OPC_SLOT_WRITE_DATA:
			fprintf( output, "%s slot data\n", message->opcode == LN_OPC_SLOT_READ_DATA ? "Read" : "Write"  );
            fprintf( output, "  Slot #: %d\n", message->slot_data.slot );
            fprintf( output, "  Slot status: 0x%X\n", message->slot_data.stat );
            loconet_print_slot_status( output, message->slot_data.stat );
            fprintf( output, "  Speed: %d\n", message->slot_data.speed );
            loconet_print_directions_and_func( output, message->slot_data.dir_funcs );
            fprintf( output, "  TRK: 0x%X\n", message->slot_data.track );
            loconet_print_track_status( output, message->slot_data.track );
            fprintf( output, "  ADDR: %d(%s)\n", message->slot_data.addr1 | (message->slot_data.addr2 << 7),
                message->slot_data.addr2 ? "LONG" : "SHORT" );
			break;
		case LN_OPC_REQUEST_SLOT_DATA:
			fprintf( output, "Request slot data\n" );
            fprintf( output, "  Slot #: %d\n", message->req_slot_data.slot );
			break;
		case LN_OPC_MOVE_SLOT:
			fprintf( output, "Move slot\n" );
            fprintf( output, "  Source slot: %d\n", message->move_slot.source );
            fprintf( output, "  Slot: %d\n", message->move_slot.slot );
			break;
		case LN_OPC_SLOT_STAT1:
			fprintf( output, "Write stat 1\n" );
			fprintf( output, "  Slot: %d\n", message->stat1.slot );
			fprintf( output, "  Stat1: 0x%X\n", message->stat1.stat1 );
			break;
		case LN_OPC_SWITCH_REQUEST:
			fprintf( output, "Switch request\n" );
            fprintf( output, "  Switch number: %d\n", message->req_switch.sw1 + 1 );
            fprintf( output, "  Set to: %s\n", message->req_switch.sw2 & ( 0x01 << 5 ) ? "CLOSED" : "THROWN" );
			break;
		default:
			fprintf( output, "Unimplemented print for opcode 0x%X\n", message->opcode );
	}
    fprintf( output, "]\n" );
}

void loconet_print_message_hex( FILE* output, const struct loconet_message* message ){
	uint8_t msgLen = message->opcode & 0xE0;

	if( msgLen == 0x80 ){
		//two bytes with checksum
		fprintf( output, "0x%02X 0x%02X\n", message->opcode, message->data[ 0 ] );
	}else if( msgLen == 0xA0 ){
		//four bytes with checksum
		fprintf( output, "0x%02X 0x%02X 0x%02X 0x%02X\n", 
			message->opcode,
			message->data[ 0 ],
			message->data[ 1 ],
			message->data[ 2 ] );
	}else if( msgLen == 0xC0 ){
		//six bytes with checksum
		fprintf( output, "0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X\n", 
			message->opcode,
			message->data[ 0 ],
			message->data[ 1 ],
			message->data[ 2 ],
			message->data[ 3 ],
			message->data[ 4 ] );
	}else if( msgLen == 0xE0 ){
		//variable length
		uint8_t thisLen = message->data[ 0 ];
		uint8_t x;
	
		if( message->opcode == 0xE7 || message->opcode == 0xEF ){
			thisLen = 12;
		}
		fprintf( output, "0x%02X ", message->opcode );
		for( x = 0; x < thisLen + 1; x++ ){
			fprintf( output, "0x%02X ", message->data[ x ] );
		}
		fprintf( output, "\n" );
	}
}
