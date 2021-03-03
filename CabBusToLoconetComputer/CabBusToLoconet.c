#include <stdio.h>
#include <string.h>

#define LOCONET_INTERLOCK

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

//internal table to keep track of which slot has which locomotive
static uint16_t slot_table[ 128 ];

struct LoconetInfoForCab {
	uint8_t slot_number;
    enum LocoRequestState request_state;
    uint16_t request_loco_number;
    uint8_t request_slot_number;
};

static struct LoconetInfoForCab cab_info[ 64 ];

//
// Local Functions
//

//
// External Functions
//

void cabbus_to_loconet_main( struct cabbus_context* cab_context,
                             cab_write_fn cab_write,
                             cabbus_read_fn cab_read,
                             void* cab_read_fn_data,
                             LoconetContext* loconet_context,
                             loconet_read_fn loconet_read,
                             void* loconet_read_fn_data){

	//local variable setup
    memset( slot_table, 0, sizeof( slot_table ) );


	// Ensure that each cab has a pointer to user data
	memset( cab_info, 0, sizeof( cab_info ) );
	for( int x = 0; x < 64; x++ ){
        struct cabbus_cab* cab = cabbus_cab_by_id( cab_context, x );
		if( !cab ) continue;
        cab_info[ x ].slot_number = 255;
        cabbus_cab_set_user_data( cab, &cab_info[ x ] );
	}

    struct cabbus_cab* cab;
    struct cab_command* cmd;
    Ln_Message outgoingMessage;
    Ln_Message incomingMessage;
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
                if( ln_write_message( loconet_context, &outgoingMessage ) < 0 ){
					fprintf( stderr, "ERROR writing outgoing message\n" );
				}
			}else if( cmd->command == CAB_CMD_SPEED ){
                struct LoconetInfoForCab* info = cabbus_cab_get_user_data( cab );
				Ln_Message message;
                message.opcode = LN_OPC_LOCO_SPEED;
				message.speed.speed = cmd->speed.speed & 0x7F;
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
            }else if( cmd->command == CAB_CMD_DIRECTION ||
                      cmd->command == CAB_CMD_FUNCTION ){
                struct LoconetInfoForCab* info = cabbus_cab_get_user_data( cab );
                Ln_Message message;
                message.opcode = LN_OPC_LOCO_DIR_FUNC;
                message.dirFunc.slot = info->slot_number;
                message.dirFunc.dir_funcs = 0;
                if( cmd->direction.direction == CAB_DIR_REVERSE ){
                    LOCONET_SET_DIRECTION_REV(message.dirFunc.dir_funcs);
                }else{
                    LOCONET_SET_DIRECTION_FWD(message.dirFunc.dir_funcs);
                }

                // First set all of the known values that we have for our functions
                message.dirFunc.dir_funcs |= (cabbus_cab_get_function( cab, 0 ) << 4);
                message.dirFunc.dir_funcs |= (cabbus_cab_get_function( cab, 1 ) << 0);
                message.dirFunc.dir_funcs |= (cabbus_cab_get_function( cab, 2 ) << 1);
                message.dirFunc.dir_funcs |= (cabbus_cab_get_function( cab, 3 ) << 2);
                message.dirFunc.dir_funcs |= (cabbus_cab_get_function( cab, 4 ) << 3);

                // Now set any functions that may have changed
                if( cmd->command == CAB_CMD_FUNCTION ){
                    if( cmd->function.function_number == 0 && cmd->function.onoff ){
                        SET_BIT(message.dirFunc.dir_funcs, 4);
                    }else{
                        CLEAR_BIT(message.dirFunc.dir_funcs, 4);
                    }

                    if( cmd->function.function_number == 1 && cmd->function.onoff ){
                        SET_BIT(message.dirFunc.dir_funcs, 0);
                    }else{
                        CLEAR_BIT(message.dirFunc.dir_funcs, 0);
                    }

                    if( cmd->function.function_number == 2 && cmd->function.onoff ){
                        SET_BIT(message.dirFunc.dir_funcs, 1);
                    }else{
                        CLEAR_BIT(message.dirFunc.dir_funcs, 1);
                    }

                    if( cmd->function.function_number == 3 && cmd->function.onoff ){
                        SET_BIT(message.dirFunc.dir_funcs, 2);
                    }else{
                        CLEAR_BIT(message.dirFunc.dir_funcs, 2);
                    }

                    if( cmd->function.function_number == 4 && cmd->function.onoff ){
                        SET_BIT(message.dirFunc.dir_funcs, 3);
                    }else{
                        CLEAR_BIT(message.dirFunc.dir_funcs, 3);
                    }
                }

                printf( "Going to write Loconet message:\n" );
                loconet_print_message( stdout, &message );
                if( ln_write_message( loconet_context, &message ) < 0 ){
                    printf( "ERROR writing message\n" );
                }
            }else if( cmd->command == CAB_CMD_RESPONSE ){
                struct LoconetInfoForCab* info = cabbus_cab_get_user_data( cab );
                if( info->request_state == STATE_STEAL &&
                        cmd->response.response ){
                    // Requested to steal OK
                    info->slot_number = info->request_slot_number;
                    // Request the slot data so that we update our status
                    Ln_Message msg;
                    msg.opcode = LN_OPC_REQUEST_SLOT_DATA;
                    msg.reqSlotData.slot = info->slot_number;
                    msg.reqSlotData.nul = 0;
                    if( ln_write_message( loconet_context, &msg ) < 0 ){
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

                    Ln_Message msg;
                    msg.opcode = LN_OPC_SLOT_STAT1;
                    msg.stat1.slot = info->slot_number;
                    msg.stat1.stat1 = (LN_SLOT_STATUS_COMMON << 4) | 0x7;
                    if( ln_write_message( loconet_context, &msg ) < 0 ){
                        printf( "ERROR writing message\n" );
                    }

                    cabbus_cab_set_loco_number( cab, 0 );
                    cabbus_cab_set_loco_speed( cab, 0 );
                }
            }
		}

        loconet_read( loconet_read_fn_data );

        if( ln_read_message( loconet_context, &incomingMessage ) == 1 ){
            printf( "Incoming loconet message:\n" );
			loconet_print_message( stdout, &incomingMessage );
			if( incomingMessage.opcode == LN_OPC_SLOT_READ_DATA ){
                //check to see if this slot is one that we are controlling
				int addr = incomingMessage.rdSlotData.addr1 | (incomingMessage.rdSlotData.addr2 << 7);
                struct LoconetInfoForCab* info;
                struct cabbus_cab* cab;
                for( int x = 0; x < ( sizeof( cab_info ) / sizeof( cab_info[ 0 ] ) ); x++ ){
                    cab = cabbus_cab_by_id( cab_context, x );
                    info = cabbus_cab_get_user_data( cab );
                    if( info == NULL ){
                        continue;
                    }
                    if( info->request_state == STATE_REQUEST &&
                            info->request_loco_number == addr ){
                        if( LN_SLOT_STATUS(incomingMessage) == LN_SLOT_STATUS_IN_USE ){
                            // Ask to steal
                            info->request_state = STATE_STEAL;
                            info->request_slot_number = incomingMessage.rdSlotData.slot;
                            cabbus_cab_ask_question( cab, "STEAL? (Y)" );
                        }else{
                            //perform a NULL MOVE
                            outgoingMessage.opcode = LN_OPC_MOVE_SLOT;
                            outgoingMessage.moveSlot.source = incomingMessage.rdSlotData.slot;
                            outgoingMessage.moveSlot.slot = incomingMessage.rdSlotData.slot;
                            info->request_state = STATE_NULL_MOVE;
                            info->slot_number = incomingMessage.rdSlotData.slot;
                            if( ln_write_message( loconet_context, &outgoingMessage ) < 0 ){
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
                            info->slot_number == incomingMessage.rdSlotData.slot ){
                        // Update everything about this guy on the cab
                        cabbus_cab_set_loco_number( cab, addr );
                        if( LOCONET_GET_DIRECTION_REV(incomingMessage.rdSlotData.dir_funcs) ){
                            cabbus_cab_set_direction( cab, CAB_DIR_REVERSE );
                        }else{
                            cabbus_cab_set_direction( cab, CAB_DIR_FORWARD );
                        }

                        char lights = !!(incomingMessage.rdSlotData.dir_funcs & (0x01 << 4));
                        char f1 = !!(incomingMessage.rdSlotData.dir_funcs & (0x01 << 0));
                        char f2 = !!(incomingMessage.rdSlotData.dir_funcs & (0x01 << 1));
                        char f3 = !!(incomingMessage.rdSlotData.dir_funcs & (0x01 << 2));
                        char f4 = !!(incomingMessage.rdSlotData.dir_funcs & (0x01 << 3));
                        cabbus_cab_set_functions( cab, 0, lights );
                        cabbus_cab_set_functions( cab, 1, f1 );
                        cabbus_cab_set_functions( cab, 2, f2 );
                        cabbus_cab_set_functions( cab, 3, f3 );
                        cabbus_cab_set_functions( cab, 4, f4 );
                    }
                }
			}else if( incomingMessage.opcode == LN_OPC_LONG_ACK ){
                if( (incomingMessage.ack.lopc & 0x7F) == LN_OPC_MOVE_SLOT ){
                    printf( "Ack? %d\n", incomingMessage.ack.ack );
                }

                struct LoconetInfoForCab* info;
                for( int x = 0; x < ( sizeof( cab_info ) / sizeof( cab_info[ 0 ] ) ); x++ ){
                    info = cabbus_cab_get_user_data( cabbus_cab_by_id( cab_context, x ) );
                    if( info == NULL ){
                        continue;
                    }
                    if( info->request_state == STATE_NULL_MOVE ){
                        info->request_state = STATE_NONE;
                    }
                }
            }else if( incomingMessage.opcode == LN_OPC_LOCO_SPEED ){
                struct LoconetInfoForCab* info;
                struct cabbus_cab* cab;
                for( int x = 0; x < ( sizeof( cab_info ) / sizeof( cab_info[ 0 ] ) ); x++ ){
                    cab = cabbus_cab_by_id( cab_context, x );
                    info = cabbus_cab_get_user_data( cab );
                    if( info == NULL ){
                        continue;
                    }

                    if( info->slot_number == incomingMessage.speed.slot ){
                        int realSpeed = incomingMessage.speed.speed - 1;
                        if( realSpeed < 0 ){
                            realSpeed = 0;
                        }
                        cabbus_cab_set_loco_speed( cab, realSpeed & 0xFF );
                    }
                }
            }else if( incomingMessage.opcode == LN_OPC_LOCO_DIR_FUNC ){
                struct LoconetInfoForCab* info;
                struct cabbus_cab* cab;
                for( int x = 0; x < ( sizeof( cab_info ) / sizeof( cab_info[ 0 ] ) ); x++ ){
                    cab = cabbus_cab_by_id( cab_context, x );
                    info = cabbus_cab_get_user_data( cab );
                    if( info == NULL ){
                        continue;
                    }

                    if( info->slot_number == incomingMessage.dirFunc.slot ){
                        if( LOCONET_GET_DIRECTION_REV(incomingMessage.dirFunc.dir_funcs) ){
                            cabbus_cab_set_direction( cab, CAB_DIR_REVERSE );
                        }else{
                            cabbus_cab_set_direction( cab, CAB_DIR_FORWARD );
                        }

                        char lights = !!(incomingMessage.dirFunc.dir_funcs & (0x01 << 4));
                        char f1 = !!(incomingMessage.dirFunc.dir_funcs & (0x01 << 0));
                        char f2 = !!(incomingMessage.dirFunc.dir_funcs & (0x01 << 1));
                        char f3 = !!(incomingMessage.dirFunc.dir_funcs & (0x01 << 2));
                        char f4 = !!(incomingMessage.dirFunc.dir_funcs & (0x01 << 3));
                        cabbus_cab_set_functions( cab, 0, lights );
                        cabbus_cab_set_functions( cab, 1, f1 );
                        cabbus_cab_set_functions( cab, 2, f2 );
                        cabbus_cab_set_functions( cab, 3, f3 );
                        cabbus_cab_set_functions( cab, 4, f4 );

                    }
                }
            }else if( incomingMessage.opcode == LN_OPC_SLOT_STAT1 ){
                struct LoconetInfoForCab* info;
                struct cabbus_cab* cab;
                for( int x = 0; x < ( sizeof( cab_info ) / sizeof( cab_info[ 0 ] ) ); x++ ){
                    cab = cabbus_cab_by_id( cab_context, x );
                    info = cabbus_cab_get_user_data( cab );
                    if( info == NULL ){
                        continue;
                    }

                    if( info->slot_number == incomingMessage.dirFunc.slot &&
                            info->request_state == STATE_SET_COMMON ){
                        // Dispatch the LOCO
                        Ln_Message msg;
                        msg.opcode = LN_OPC_MOVE_SLOT;
                        msg.moveSlot.source = info->slot_number;
                        msg.moveSlot.slot = 0;
                        if( ln_write_message( loconet_context, &msg ) < 0 ){
                            printf( "ERROR writing message\n" );
                        }

                        info->slot_number = 255;
                        info->request_state = STATE_NONE;

                        cabbus_cab_set_loco_number( cab, 0 );
                        cabbus_cab_set_loco_speed( cab, 0 );
                    }
                }
            }
		}

        fflush( stdout );
        fflush( stderr );
		usleep( 1000 );
	}
}
