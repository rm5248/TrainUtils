#include <stdio.h>

#include "loconet_print.h"

static void loconet_print_clock( FILE* output, const struct loconet_message* message ){
    fprintf( output, "%s slot data\n", message->opcode == LN_OPC_SLOT_READ_DATA ? "Read" : "Write"  );
    fprintf( output, "  Slot #: %d(clock)\n", message->clock_slot_data.slot );
    fprintf( output, "  Clock rate: %d\n", message->clock_slot_data.clock_rate );
    fprintf( output, "  Frac_minsh: %d\n", message->clock_slot_data.frac_minsh );
    fprintf( output, "  Frac_minsl: %d\n", message->clock_slot_data.frac_minsl );
    fprintf( output, "  Frac mins: %d\n", (message->clock_slot_data.frac_minsh << 8) |
             message->clock_slot_data.frac_minsl );
    fprintf( output, "  minutes: %d\n", message->clock_slot_data.mins_60 );
    fprintf( output, "  hours: %d\n", message->clock_slot_data.hours_24 );
    fprintf( output, "  Days: %d\n", message->clock_slot_data.days );
    fprintf( output, "  Clock control: %d\n", message->clock_slot_data.clock_ctl );

    int hours = ((0xFF - message->clock_slot_data.hours_24) & 0x7F) % 24;
    hours = (24 - hours) % 24;
    int minutes = ((0xFF - message->clock_slot_data.mins_60) & 0x7F) % 60;
    minutes = (60 - minutes) % 60;
    fprintf( output, "  Time is: %d:%02d\n", hours - 1, minutes );
}

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

static void loconet_print_high_functions( FILE* output, uint8_t byte ){
    fprintf( output, "  Functions: " );
    if( byte & 0x01 ) fprintf( output, "F5 " );
    if( byte & 0x02 ) fprintf( output, "F6 " );
    if( byte & 0x04 ) fprintf( output, "F7 " );
    if( byte & 0x08 ) fprintf( output, "F8 " );
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
        if( message->slot_data.slot == 123 ){
            loconet_print_clock( output, message );
            break;
        }
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
    case LN_OPC_LOCO_SOUND:
        fprintf( output, "Sound functions\n" );
        fprintf( output, "  Slot: %d\n", message->sound.slot );
        loconet_print_high_functions( output, message->sound.snd );
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

static int loconet_dump_bytes(char * output_string,
                              size_t output_string_len,
                              const struct loconet_message* message){
    uint8_t msgLen = message->opcode & 0xE0;

    if( msgLen == 0x80 ){
        //two bytes with checksum
        return snprintf(output_string,
                        output_string_len,
                        "[%02X %02X] ",
                        message->opcode,
                        message->data[ 0 ]);
    }else if( msgLen == 0xA0 ){
        //four bytes with checksum
        return snprintf(output_string,
                        output_string_len,
                        "[%02X %02X %02X %02X] ",
                        message->opcode,
                        message->data[ 0 ],
                        message->data[ 1 ],
                        message->data[ 2 ]);
    }else if( msgLen == 0xC0 ){
        //six bytes with checksum
        return snprintf(output_string,
                        output_string_len,
                        "[%02X %02X %02X %02X %02X %02X] ",
                        message->opcode,
                        message->data[ 0 ],
                        message->data[ 1 ],
                        message->data[ 2 ],
                        message->data[ 3 ],
                        message->data[ 4 ]);
    }else if( msgLen == 0xE0 ){
        //variable length
        uint8_t thisLen = message->data[ 0 ];
        uint8_t x;
        size_t max_to_write = output_string_len;
        int ret;
        char* next_byte_location = output_string;

        if( message->opcode == 0xE7 || message->opcode == 0xEF ){
            thisLen = 12;
        }
        ret = snprintf(next_byte_location, max_to_write, "[%02X", message->opcode);
        max_to_write -= ret;
        next_byte_location += ret;
        for( x = 0; x < thisLen + 1; x++ ){
            ret = snprintf(next_byte_location, max_to_write, " %02X", message->data[x]);
            max_to_write -= ret;
            next_byte_location += ret;
        }
        ret = snprintf(next_byte_location, max_to_write, "] ");
        next_byte_location += ret;

        return next_byte_location - output_string;
    }

    return 0;
}

static int loconet_dump_slot_status( char * output_string,
                                     size_t output_string_len,
                                     uint8_t stat ){
    uint8_t decoderType = stat & 0x07;
    uint8_t slotStatus = ( stat & ( 0x03 << 4 ) ) >> 4;
    uint8_t consistStatus = ( stat & ( 0x03 << 6 ) ) >> 6;
    size_t max_to_write = output_string_len;
    int ret;
    char* next_byte_location = output_string;

    if( decoderType == 0x00 ){
        ret = snprintf(next_byte_location, max_to_write, "  28 step/ 3byte packet regular mode\n");
        max_to_write -= ret;
        next_byte_location += ret;
    }else if( decoderType == 0x01 ){
        ret = snprintf(next_byte_location, max_to_write, "  28 step/ trinary packets\n");
        max_to_write -= ret;
        next_byte_location += ret;
    }else if( decoderType == 0x02 ){
        ret = snprintf(next_byte_location, max_to_write, "  14 step mode\n");
        max_to_write -= ret;
        next_byte_location += ret;
    }else if( decoderType == 0x03 ){
        ret = snprintf(next_byte_location, max_to_write, "  128 mode packets\n");
        max_to_write -= ret;
        next_byte_location += ret;
    }else if( decoderType == 0x04 ){
        ret = snprintf(next_byte_location, max_to_write, "  28 step, advanced DCC consisting\n");
        max_to_write -= ret;
        next_byte_location += ret;
    }else if( decoderType == 0x07 ){
        ret = snprintf(next_byte_location, max_to_write, "  128 step, advanced DCC consisting\n");
        max_to_write -= ret;
        next_byte_location += ret;
    }else{
        ret = snprintf(next_byte_location, max_to_write, "  Unkown DCC decoder type\n");
        max_to_write -= ret;
        next_byte_location += ret;
    }

    if( slotStatus == 0x00 ){
        ret = snprintf(next_byte_location, max_to_write, "  Slot status: FREE\n");
        max_to_write -= ret;
        next_byte_location += ret;
    }else if( slotStatus == 0x01 ){
        ret = snprintf(next_byte_location, max_to_write, "  Slot status: COMMON\n");
        max_to_write -= ret;
        next_byte_location += ret;
    }else if( slotStatus == 0x02 ){
        ret = snprintf(next_byte_location, max_to_write, "  Slot status: IDLE\n");
        max_to_write -= ret;
        next_byte_location += ret;
    }else if( slotStatus == 0x03 ){
        ret = snprintf(next_byte_location, max_to_write, "  Slot status: IN_USE\n");
        max_to_write -= ret;
        next_byte_location += ret;
    }

    if( consistStatus == 0x00 ){
        ret = snprintf(next_byte_location, max_to_write, "  Consist status: FREE, no consist\n");
        max_to_write -= ret;
        next_byte_location += ret;
    }else if( consistStatus == 0x01 ){
        ret = snprintf(next_byte_location, max_to_write, "  Consist status: logical consist sub-membert\n");
        max_to_write -= ret;
        next_byte_location += ret;
    }else if( consistStatus == 0x02 ){
        ret = snprintf(next_byte_location, max_to_write, "  Consist status: logical consist top\n");
        max_to_write -= ret;
        next_byte_location += ret;
    }else if( consistStatus == 0x03 ){
        ret = snprintf(next_byte_location, max_to_write, "    Consist status: logical mid consist\n");
        max_to_write -= ret;
        next_byte_location += ret;
    }

    return next_byte_location - output_string;
}

static int loconet_dump_track_status( char * output_string,
                                       size_t output_string_len,
                                       uint8_t trk ){
    size_t max_to_write = output_string_len;
    int ret;
    char* next_byte_location = output_string;

    if( trk & 0x01 ){
        ret = snprintf(next_byte_location, max_to_write, "  DCC Power ON, Global power UP\n");
        max_to_write -= ret;
        next_byte_location += ret;
    }else{
        ret = snprintf(next_byte_location, max_to_write, "  DCC Power OFF\n");
        max_to_write -= ret;
        next_byte_location += ret;
    }

    if( !(trk & 0x02) ){
        ret = snprintf(next_byte_location, max_to_write, "  Track PAUSED, Broadcast ESTOP\n");
        max_to_write -= ret;
        next_byte_location += ret;
    }

    if( trk & 0x04 ){
        ret = snprintf(next_byte_location, max_to_write, "  Master has LocoNet 1.1\n");
        max_to_write -= ret;
        next_byte_location += ret;
    }else{
        ret = snprintf(next_byte_location, max_to_write, "  Master is DT200\n");
        max_to_write -= ret;
        next_byte_location += ret;
    }

    if( trk & 0x08 ){
        ret = snprintf(next_byte_location, max_to_write, "  Programming Track Busy\n");
        max_to_write -= ret;
        next_byte_location += ret;
    }

    return next_byte_location - output_string;
}

static int loconet_dump_directions_and_func( char * output_string,
                                             size_t output_string_len,
                                             uint8_t byte ){
    size_t max_to_write = output_string_len;
    int ret;
    char* next_byte_location = output_string;

    ret = snprintf(next_byte_location, max_to_write,
                   "Direction: %s Functions: %s%s%s%s%s",
                   byte & 0x20 ? "REV" : "FWD",
                   byte & 0x10 ? "F0 " : "",
                   byte & 0x01 ? "F1 " : "",
                   byte & 0x02 ? "F2 " : "",
                   byte & 0x04 ? "F3 " : "",
                   byte & 0x08 ? "F4 " : "");
    max_to_write -= ret;
    next_byte_location += ret;

    return next_byte_location - output_string;
}

void loconet_message_decode_as_str(char* output_string,
                                   size_t output_string_len,
                                   const struct loconet_message* message,
                                   int flags){
    char* next_byte_location = output_string;
    size_t max_to_write = output_string_len;
    int ret;

    if(flags & LOCONET_PRINT_FLAG_DISPLAY_BYTES){
        ret = loconet_dump_bytes(output_string, output_string_len, message);
        max_to_write -= ret;
        next_byte_location += ret;
    }

    switch( message->opcode ){
    case LN_OPC_LOCO_SPEED:
        snprintf(next_byte_location, max_to_write, "Set locomotive speed in slot %d to %d",
                 message->speed.slot,
                 message->speed.speed);
        break;
    case LN_OPC_SWITCH_REQUEST:
        snprintf(next_byte_location, max_to_write, "Switch %d set to %s",
                 message->req_switch.sw1 + 1,
                 message->req_switch.sw2 & ( 0x01 << 5 ) ? "CLOSED" : "THROWN");
        break;
    case LN_OPC_REQUEST_SLOT_DATA:
        snprintf(next_byte_location, max_to_write, "Request slot data for slot %d", message->req_slot_data.slot );
        break;
    case LN_OPC_MOVE_SLOT:
        snprintf(next_byte_location, max_to_write, "Move slot %d to %d", message->move_slot.source, message->move_slot.slot );
        break;
    case LN_OPC_SLOT_STAT1:
        snprintf(next_byte_location, max_to_write, "Write slot %d stat1 0x%02X", message->stat1.slot, message->stat1.stat1 );
        break;
    case LN_OPC_SLOT_READ_DATA:
    case LN_OPC_SLOT_WRITE_DATA:
    if( message->slot_data.slot == 123 ){
        snprintf(next_byte_location, max_to_write, "clock data");
//        loconet_print_clock( output, message );
        break;
    }
        ret = snprintf(next_byte_location, max_to_write, "%s slot data\n", message->opcode == LN_OPC_SLOT_READ_DATA ? "Read" : "Write"  );
        max_to_write -= ret;
        next_byte_location += ret;

        ret = snprintf(next_byte_location, max_to_write, "  Slot #: %d\n", message->slot_data.slot );
        max_to_write -= ret;
        next_byte_location += ret;
        ret = snprintf(next_byte_location, max_to_write, "  Slot status: 0x%X\n", message->slot_data.stat );
        max_to_write -= ret;
        next_byte_location += ret;
        ret = loconet_dump_slot_status( next_byte_location, max_to_write, message->slot_data.stat );
        max_to_write -= ret;
        next_byte_location += ret;
        ret = snprintf(next_byte_location, max_to_write, "  Speed: %d\n", message->slot_data.speed );
        max_to_write -= ret;
        next_byte_location += ret;
        ret = loconet_dump_directions_and_func( next_byte_location, max_to_write, message->slot_data.dir_funcs );
        max_to_write -= ret;
        next_byte_location += ret;
        ret = snprintf(next_byte_location, max_to_write, "  TRK: 0x%X\n", message->slot_data.track );
        max_to_write -= ret;
        next_byte_location += ret;
        ret = loconet_dump_track_status( next_byte_location, max_to_write, message->slot_data.track );
        max_to_write -= ret;
        next_byte_location += ret;
        ret = snprintf(next_byte_location, max_to_write, "  ADDR: %d(%s)\n", message->slot_data.addr1 | (message->slot_data.addr2 << 7),
            message->slot_data.addr2 ? "LONG" : "SHORT" );
        break;

    case LN_OPC_LOCO_DIR_FUNC:
        ret = snprintf(next_byte_location, max_to_write, "Locomotive direction/funcs. Slot: %d ", message->direction_functions.slot );
        max_to_write -= ret;
        next_byte_location += ret;
        loconet_dump_directions_and_func( next_byte_location, max_to_write, message->direction_functions.dir_funcs );
        break;
    case LN_OPC_LONG_ACK:
        ret = snprintf(next_byte_location, max_to_write, "LACK Opcode: 0x%02X Status: 0x%02X Fail? %s",
                       message->ack.lopc,
                       message->ack.ack,
                       message->ack.ack == 0 ? "true" : "false" );
        break;
    case LN_OPC_LOCO_ADDR:
        ret = snprintf(next_byte_location, max_to_write, "Request locomotive address %d",
                       message->addr.locoAddrHi << 7 | message->addr.locoAddrLo );
        break;
    case LN_OPC_POWER_ON:
        ret = snprintf(next_byte_location, max_to_write, "Track power ON" );
        break;
    case LN_OPC_POWER_OFF:
        ret = snprintf(next_byte_location, max_to_write, "Track power OFF" );
        break;
    }
}
