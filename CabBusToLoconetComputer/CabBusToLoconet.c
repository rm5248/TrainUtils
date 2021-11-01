#include <stdio.h>
#include <string.h>

#define LOCONET_INTERLOCK

#define DIRECTION_FWD 0
#define DIRECTION_REV 1

#include "CabBus.h"
#include "loconet_buffer.h"
#include "loconet_print.h"
#include "cabbus_to_loconet.h"

enum LocoRequestState{
    STATE_NONE,
    STATE_REQUEST,
    STATE_STEAL,
    STATE_NULL_MOVE,
    STATE_ACK,
    STATE_SET_COMMON,
};

//
// Local Variables
//

struct LoconetInfoForCab {
	uint8_t slot_number;
    enum LocoRequestState request_state;
    uint16_t request_loco_number;
    uint8_t request_slot_number;
    uint8_t direction;
};

struct cab2loconet_context {
    struct cabbus_context* cab_context;
    cab_write_fn cab_write;
    cabbus_read_fn cab_read;
    void* cab_read_fn_data;
    struct loconet_context* loconet_context;
    loconet_read_fn loconet_read;
    void* loconet_read_fn_data;

    struct LoconetInfoForCab cab_info[ 64 ];
};

//
// Local Functions
//

static void handle_cabbus_speed( struct LoconetInfoForCab* info,
                                 struct loconet_context* loconet_context,
                                 int speed,
                                 int estop ){
    struct loconet_message message;
    message.opcode = LN_OPC_LOCO_SPEED;
    message.speed.speed = speed & 0x7F;
    /*
     * Special logic for Digitrax : speed step 0 is stopped,
     * while speed step 1 is ESTOP.
     * So, add 1 to each speed we get from the cabbus.
     * if the speed is 1, that means we are now stopped
     */
    message.speed.speed++;
    if( message.speed.speed == 1 ){
        message.speed.speed = 0;
    }

    if( estop == 1 ){
        message.speed.speed = 1;
    }

    message.speed.slot = info->slot_number;
    if( info->slot_number == 255 ){
        printf( "ERROR no slot number\n" );
    }else{
        printf( "Going to write Loconet message:\n" );
        loconet_print_message( stdout, &message );
        if( ln_write_message( loconet_context, &message ) < 0 ){
            printf( "ERROR writing message\n" );
        }
    }
}

static void handle_good_cab_ping( struct cab2loconet_context* context, struct cabbus_cab* cab ){
    struct cab_command* cmd;
    struct loconet_message outgoingMessage;

    cmd = cabbus_cab_get_command( cab );
    if( cmd->command != CAB_CMD_NONE ){
        printf( "Got command %d[%s]\n",
            cmd->command,
            cabbus_command_to_string(cmd->command) );
    }
    if( cmd->command == CAB_CMD_SEL_LOCO ){
        printf( "Select loco %d(long? %d)\n",
            cmd->sel_loco.address,
            cmd->sel_loco.flags );
        // Send this data to loconet.
        struct LoconetInfoForCab* info = cabbus_cab_get_user_data( cab );
        outgoingMessage.opcode = LN_OPC_LOCO_ADDR;
        outgoingMessage.addr.locoAddrLo = cmd->sel_loco.address & 0x7F;
        outgoingMessage.addr.locoAddrHi = (cmd->sel_loco.address & ~0x7F) >> 7;
        info->request_state = STATE_REQUEST;
        info->request_loco_number = cmd->sel_loco.address;
        loconet_print_message( stdout, &outgoingMessage );
        if( ln_write_message( context->loconet_context, &outgoingMessage ) < 0 ){
            fprintf( stderr, "ERROR writing outgoing message\n" );
        }
    }else if( cmd->command == CAB_CMD_SPEED ||
              cmd->command == CAB_CMD_ESTOP ){
        struct LoconetInfoForCab* info = cabbus_cab_get_user_data( cab );
        handle_cabbus_speed( info,
                             context->loconet_context,
                             cmd->speed.speed & 0x7F,
                             cmd->command == CAB_CMD_ESTOP );
    }else if( cmd->command == CAB_CMD_DIRECTION ||
              cmd->command == CAB_CMD_FUNCTION ){
        struct LoconetInfoForCab* info = cabbus_cab_get_user_data( cab );
        struct loconet_message message;
        message.opcode = LN_OPC_LOCO_DIR_FUNC;
        message.direction_functions.slot = info->slot_number;
        message.direction_functions.dir_funcs = 0;

        if( cmd->command == CAB_CMD_DIRECTION ){
            if( cmd->direction.direction == CAB_DIR_FORWARD ){
                info->direction = DIRECTION_FWD;
            }else{
                info->direction = DIRECTION_REV;
            }
        }
        if( info->direction == DIRECTION_REV ){
            LOCONET_SET_DIRECTION_REV(message.direction_functions.dir_funcs);
        }else{
            LOCONET_SET_DIRECTION_FWD(message.direction_functions.dir_funcs);
        }

        // First set all of the known values that we have for our functions
        message.direction_functions.dir_funcs |= (cabbus_cab_get_function( cab, 0 ) << 4);
        message.direction_functions.dir_funcs |= (cabbus_cab_get_function( cab, 1 ) << 0);
        message.direction_functions.dir_funcs |= (cabbus_cab_get_function( cab, 2 ) << 1);
        message.direction_functions.dir_funcs |= (cabbus_cab_get_function( cab, 3 ) << 2);
        message.direction_functions.dir_funcs |= (cabbus_cab_get_function( cab, 4 ) << 3);

        // Set any functions that may have changed,
        // and tell the cab that the function is on(or off)
        if( cmd->command == CAB_CMD_FUNCTION ){
            if( cmd->function.function_number == 0 && cmd->function.onoff ){
                SET_BIT(message.direction_functions.dir_funcs, 4);
                cabbus_cab_set_functions( cab, 0, 1 );
            }else if( cmd->function.function_number == 0 ){
                CLEAR_BIT(message.direction_functions.dir_funcs, 4);
                cabbus_cab_set_functions( cab, 0, 0 );
            }

            if( cmd->function.function_number == 1 && cmd->function.onoff ){
                SET_BIT(message.direction_functions.dir_funcs, 0);
                cabbus_cab_set_functions( cab, 1, 1 );
            }else if( cmd->function.function_number == 1 ){
                CLEAR_BIT(message.direction_functions.dir_funcs, 0);
                cabbus_cab_set_functions( cab, 1, 0 );
            }

            if( cmd->function.function_number == 2 && cmd->function.onoff ){
                SET_BIT(message.direction_functions.dir_funcs, 1);
                cabbus_cab_set_functions( cab, 2, 1 );
            }else if( cmd->function.function_number == 2 ){
                CLEAR_BIT(message.direction_functions.dir_funcs, 1);
                cabbus_cab_set_functions( cab, 2, 0 );
            }

            if( cmd->function.function_number == 3 && cmd->function.onoff ){
                SET_BIT(message.direction_functions.dir_funcs, 2);
                cabbus_cab_set_functions( cab, 3, 1 );
            }else if( cmd->function.function_number == 3 ){
                CLEAR_BIT(message.direction_functions.dir_funcs, 2);
                cabbus_cab_set_functions( cab, 3, 0 );
            }

            if( cmd->function.function_number == 4 && cmd->function.onoff ){
                SET_BIT(message.direction_functions.dir_funcs, 3);
                cabbus_cab_set_functions( cab, 4, 1 );
            }else if( cmd->function.function_number == 4 ){
                CLEAR_BIT(message.direction_functions.dir_funcs, 3);
                cabbus_cab_set_functions( cab, 4, 0 );
            }

            if( cmd->function.function_number >= 5 && cmd->function.function_number <= 8 ){
                message.opcode = LN_OPC_LOCO_SOUND;
                message.sound.snd = 0;
                message.sound.snd |= (cabbus_cab_get_function( cab, 5 ) << 0);
                message.sound.snd |= (cabbus_cab_get_function( cab, 6 ) << 1);
                message.sound.snd |= (cabbus_cab_get_function( cab, 7 ) << 2);
                message.sound.snd |= (cabbus_cab_get_function( cab, 8 ) << 3);

                if( cmd->function.function_number == 5 && cmd->function.onoff ){
                    SET_BIT(message.sound.snd, 0);
                    cabbus_cab_set_functions( cab, 5, 1 );
                }else if( cmd->function.function_number == 5 ){
                    CLEAR_BIT(message.sound.snd, 0);
                    cabbus_cab_set_functions( cab, 5, 0 );
                }

                if( cmd->function.function_number == 6 && cmd->function.onoff ){
                    SET_BIT(message.sound.snd, 1);
                    cabbus_cab_set_functions( cab, 6, 1 );
                }else if( cmd->function.function_number == 6 ){
                    CLEAR_BIT(message.sound.snd, 1);
                    cabbus_cab_set_functions( cab, 6, 0 );
                }

                if( cmd->function.function_number == 7 && cmd->function.onoff ){
                    SET_BIT(message.sound.snd, 2);
                    cabbus_cab_set_functions( cab, 7, 1 );
                }else if( cmd->function.function_number == 7 ){
                    CLEAR_BIT(message.sound.snd, 2);
                    cabbus_cab_set_functions( cab, 7, 0 );
                }

                if( cmd->function.function_number == 8 && cmd->function.onoff ){
                    SET_BIT(message.sound.snd, 3);
                    cabbus_cab_set_functions( cab, 8, 1 );
                }else if( cmd->function.function_number == 8 ){
                    CLEAR_BIT(message.sound.snd, 3);
                    cabbus_cab_set_functions( cab, 8, 0 );
                }
            }
        }

        printf( "Going to write Loconet message:\n" );
        loconet_print_message( stdout, &message );
        if( ln_write_message( context->loconet_context, &message ) < 0 ){
            printf( "ERROR writing message\n" );
        }
    }else if( cmd->command == CAB_CMD_RESPONSE ){
        struct LoconetInfoForCab* info = cabbus_cab_get_user_data( cab );
        if( info->request_state == STATE_STEAL &&
                cmd->response.response ){
            // Requested to steal OK
            info->slot_number = info->request_slot_number;
            // Request the slot data so that we update our status
            struct loconet_message msg;
            msg.opcode = LN_OPC_REQUEST_SLOT_DATA;
            msg.req_slot_data.slot = info->slot_number;
            msg.req_slot_data.nul = 0;
            if( ln_write_message( context->loconet_context, &msg ) < 0 ){
                printf( "ERROR writing message\n" );
            }
        }
        info->request_state = STATE_NONE;
    }else if( cmd->command == CAB_CMD_UNSELECT_LOCO ){
        struct LoconetInfoForCab* info = cabbus_cab_get_user_data( cab );
        if( info->slot_number != 255 ){
            // Two step process:
            // 1. set slot to COMMON
            // 2. Move to slot 0(DISPATCH)
            // Dispatch the slot
            info->request_state = STATE_SET_COMMON;

            struct loconet_message msg;
            msg.opcode = LN_OPC_SLOT_STAT1;
            msg.stat1.slot = info->slot_number;
            msg.stat1.stat1 = (LN_SLOT_STATUS_COMMON << 4) | 0x7;
            if( ln_write_message( context->loconet_context, &msg ) < 0 ){
                printf( "ERROR writing message\n" );
            }

            cabbus_cab_set_loco_number( cab, 0 );
            cabbus_cab_set_loco_speed( cab, 0 );
        }
    }else if( cmd->command == CAB_CMD_SWITCH ){
        struct loconet_message msg;
        msg.opcode = LN_OPC_SWITCH_REQUEST;
        msg.req_switch.sw1 = cmd->switch_state.switch_number - 1;
        msg.req_switch.sw2 = cmd->switch_state.normal_rev == CAB_SWITCH_NORMAL ? ( 0x01 << 5 ) : 0;
        msg.req_switch.sw2 |= (0x01 << 4);
        if( ln_write_message( context->loconet_context, &msg ) < 0 ){
            printf( "ERROR writing message\n" );
        }
    }
}

static void handle_incoming_loconet( struct cab2loconet_context* context, struct loconet_message* incomingMessage ){
    struct cabbus_cab* cab;
    struct loconet_message outgoingMessage;

    if( incomingMessage->opcode == LN_OPC_SLOT_READ_DATA ){
        if( incomingMessage->slot_data.slot == 123 ){
            // time has just been updated.
            struct loconet_time lnTime = ln_current_time( context->loconet_context );
            int hours = lnTime.hours;
            int am = hours < 12;
            if( hours == 0 ){
                hours = 12;
            }

            cabbus_set_all_cab_times( context->cab_context, hours, lnTime.minutes, am );
            return;
        }

        //check to see if this slot is one that we are controlling
        int addr = incomingMessage->slot_data.addr1 | (incomingMessage->slot_data.addr2 << 7);
        struct LoconetInfoForCab* info;
        struct cabbus_cab* cab;
        for( int x = 0; x < ( sizeof( context->cab_info ) / sizeof( context->cab_info[ 0 ] ) ); x++ ){
            cab = cabbus_cab_by_id( context->cab_context, x );
            info = cabbus_cab_get_user_data( cab );
            if( info == NULL ){
                continue;
            }
            if( info->request_state == STATE_REQUEST &&
                    info->request_loco_number == addr ){
                if( LN_SLOT_STATUS((*incomingMessage)) == LN_SLOT_STATUS_IN_USE ){
                    // Ask to steal
                    info->request_state = STATE_STEAL;
                    info->request_slot_number = incomingMessage->slot_data.slot;
                    cabbus_cab_ask_question( cab, "STEAL? (Y)" );
                }else{
                    //perform a NULL MOVE
                    outgoingMessage.opcode = LN_OPC_MOVE_SLOT;
                    outgoingMessage.move_slot.source = incomingMessage->slot_data.slot;
                    outgoingMessage.move_slot.slot = incomingMessage->slot_data.slot;
                    info->request_state = STATE_NULL_MOVE;
                    info->slot_number = incomingMessage->slot_data.slot;
                    if( ln_write_message( context->loconet_context, &outgoingMessage ) < 0 ){
                        fprintf( stderr, "ERROR writing outgoing message\n" );
                    }
                }
                break;
            }

            if( info->request_state == STATE_NULL_MOVE &&
                    info->request_loco_number == addr ){
                // We have successfully done the NULL MOVE! :D
                // Now tell the cabbus that we have done so.
                cabbus_cab_set_loco_number( cab, info->request_loco_number );
            }

            if( info->request_state == STATE_NONE &&
                    info->slot_number == incomingMessage->slot_data.slot ){
                // Update everything about this guy on the cab
                cabbus_cab_set_loco_number( cab, addr );
                if( LOCONET_GET_DIRECTION_REV(incomingMessage->slot_data.dir_funcs) ){
                    cabbus_cab_set_direction( cab, CAB_DIR_REVERSE );
                }else{
                    cabbus_cab_set_direction( cab, CAB_DIR_FORWARD );
                }

                char lights = !!(incomingMessage->slot_data.dir_funcs & (0x01 << 4));
                char f1 = !!(incomingMessage->slot_data.dir_funcs & (0x01 << 0));
                char f2 = !!(incomingMessage->slot_data.dir_funcs & (0x01 << 1));
                char f3 = !!(incomingMessage->slot_data.dir_funcs & (0x01 << 2));
                char f4 = !!(incomingMessage->slot_data.dir_funcs & (0x01 << 3));
                cabbus_cab_set_functions( cab, 0, lights );
                cabbus_cab_set_functions( cab, 1, f1 );
                cabbus_cab_set_functions( cab, 2, f2 );
                cabbus_cab_set_functions( cab, 3, f3 );
                cabbus_cab_set_functions( cab, 4, f4 );
            }
        }
    }else if( incomingMessage->opcode == LN_OPC_LONG_ACK ){
        if( (incomingMessage->ack.lopc & 0x7F) == LN_OPC_MOVE_SLOT ){
            printf( "Ack? %d\n", incomingMessage->ack.ack );
        }

        struct LoconetInfoForCab* info;
        for( int x = 0; x < ( sizeof( context->cab_info ) / sizeof( context->cab_info[ 0 ] ) ); x++ ){
            info = cabbus_cab_get_user_data( cabbus_cab_by_id( context->cab_context, x ) );
            if( info == NULL ){
                continue;
            }
            if( info->request_state == STATE_NULL_MOVE ){
                info->request_state = STATE_NONE;
            }
        }
    }else if( incomingMessage->opcode == LN_OPC_LOCO_SPEED ){
        struct LoconetInfoForCab* info;
        struct cabbus_cab* cab;
        for( int x = 0; x < ( sizeof( context->cab_info ) / sizeof( context->cab_info[ 0 ] ) ); x++ ){
            cab = cabbus_cab_by_id( context->cab_context, x );
            info = cabbus_cab_get_user_data( cab );
            if( info == NULL ){
                continue;
            }

            if( info->slot_number == incomingMessage->speed.slot ){
                int realSpeed = incomingMessage->speed.speed - 1;
                if( realSpeed < 0 ){
                    realSpeed = 0;
                }
                cabbus_cab_set_loco_speed( cab, realSpeed & 0xFF );
            }
        }
    }else if( incomingMessage->opcode == LN_OPC_LOCO_DIR_FUNC ){
        struct LoconetInfoForCab* info;
        struct cabbus_cab* cab;
        for( int x = 0; x < ( sizeof( context->cab_info ) / sizeof( context->cab_info[ 0 ] ) ); x++ ){
            cab = cabbus_cab_by_id( context->cab_context, x );
            info = cabbus_cab_get_user_data( cab );
            if( info == NULL ){
                continue;
            }

            if( info->slot_number == incomingMessage->direction_functions.slot ){
                if( LOCONET_GET_DIRECTION_REV(incomingMessage->direction_functions.dir_funcs) ){
                    cabbus_cab_set_direction( cab, CAB_DIR_REVERSE );
                }else{
                    cabbus_cab_set_direction( cab, CAB_DIR_FORWARD );
                }

                char lights = !!(incomingMessage->direction_functions.dir_funcs & (0x01 << 4));
                char f1 = !!(incomingMessage->direction_functions.dir_funcs & (0x01 << 0));
                char f2 = !!(incomingMessage->direction_functions.dir_funcs & (0x01 << 1));
                char f3 = !!(incomingMessage->direction_functions.dir_funcs & (0x01 << 2));
                char f4 = !!(incomingMessage->direction_functions.dir_funcs & (0x01 << 3));
                cabbus_cab_set_functions( cab, 0, lights );
                cabbus_cab_set_functions( cab, 1, f1 );
                cabbus_cab_set_functions( cab, 2, f2 );
                cabbus_cab_set_functions( cab, 3, f3 );
                cabbus_cab_set_functions( cab, 4, f4 );

            }
        }
    }else if( incomingMessage->opcode == LN_OPC_SLOT_STAT1 ){
        struct LoconetInfoForCab* info;
        struct cabbus_cab* cab;
        for( int x = 0; x < ( sizeof( context->cab_info ) / sizeof( context->cab_info[ 0 ] ) ); x++ ){
            cab = cabbus_cab_by_id( context->cab_context, x );
            info = cabbus_cab_get_user_data( cab );
            if( info == NULL ){
                continue;
            }

            if( info->slot_number == incomingMessage->direction_functions.slot &&
                    info->request_state == STATE_SET_COMMON ){
                // Dispatch the LOCO
                struct loconet_message msg;
                msg.opcode = LN_OPC_MOVE_SLOT;
                msg.move_slot.source = info->slot_number;
                msg.move_slot.slot = 0;
                if( ln_write_message( context->loconet_context, &msg ) < 0 ){
                    printf( "ERROR writing message\n" );
                }

                info->slot_number = 255;
                info->request_state = STATE_NONE;

                cabbus_cab_set_loco_number( cab, 0 );
                cabbus_cab_set_loco_speed( cab, 0 );
            }
        }
    }else if( incomingMessage->opcode == LN_OPC_LOCO_SOUND ){
        struct LoconetInfoForCab* info;
        struct cabbus_cab* cab;
        for( int x = 0; x < ( sizeof( context->cab_info ) / sizeof( context->cab_info[ 0 ] ) ); x++ ){
            cab = cabbus_cab_by_id( context->cab_context, x );
            info = cabbus_cab_get_user_data( cab );
            if( info == NULL ){
                continue;
            }

            if( info->slot_number == incomingMessage->sound.slot ){

                char f5 = !!(incomingMessage->sound.snd & (0x01 << 0));
                char f6 = !!(incomingMessage->sound.snd & (0x01 << 1));
                char f7 = !!(incomingMessage->sound.snd & (0x01 << 2));
                char f8 = !!(incomingMessage->sound.snd & (0x01 << 3));
                cabbus_cab_set_functions( cab, 5, f5 );
                cabbus_cab_set_functions( cab, 6, f6 );
                cabbus_cab_set_functions( cab, 7, f7 );
                cabbus_cab_set_functions( cab, 8, f8 );

            }
        }
    }
}

//
// External Functions
//

void cabbus_to_loconet_main( struct cabbus_context* cab_context,
                             cab_write_fn cab_write,
                             cabbus_read_fn cab_read,
                             void* cab_read_fn_data,
                             struct loconet_context* loconet_context,
                             loconet_read_fn loconet_read,
                             void* loconet_read_fn_data){
    struct cab2loconet_context context;

    memset( &context, 0, sizeof( context ) );
    context.cab_context = cab_context;
    context.cab_write = cab_write;
    context.cab_read = cab_read;
    context.cab_read_fn_data = cab_read_fn_data;
    context.loconet_context = loconet_context;
    context.loconet_read = loconet_read;
    context.loconet_read_fn_data = loconet_read_fn_data;

    // Ensure that each cab has a pointer to user data
	for( int x = 0; x < 64; x++ ){
        struct cabbus_cab* cab = cabbus_cab_by_id( cab_context, x );
		if( !cab ) continue;
        context.cab_info[ x ].slot_number = 255;
        cabbus_cab_set_user_data( cab, &context.cab_info[ x ] );
	}

    struct cabbus_cab* cab;
    struct cab_command* cmd;
    struct loconet_message outgoingMessage;
    struct loconet_message incomingMessage;
	//go into our main loop.
	//essentially what we do here, is we get information from the cabs on the bus,
	//and then echo that information back onto loconet.
	//we also have to parse loconet information that we get back to make sure
	//that we tell the user about stupid stuff that they are doing
	while( 1 ){
        cabbus_ping_step1( cab_context );
        cab_read( cab_read_fn_data );
        cab = cabbus_ping_step2( cab_context );

		if( cab != NULL ){
//			printf( "got response from cab %d\n", cabbus_get_cab_number( cab ) );
            handle_good_cab_ping( &context, cab );
		}

        loconet_read( loconet_read_fn_data );

        if( ln_read_message( loconet_context, &incomingMessage ) == 1 ){
            printf( "Incoming loconet message:\n" );
			loconet_print_message( stdout, &incomingMessage );
            handle_incoming_loconet( &context, &incomingMessage );
		}

        fflush( stdout );
        fflush( stderr );
		usleep( 1000 );
	}
}
