#include <stdio.h>
#include <string.h>
#include <cserial/c_serial.h>

//linux includes here
#include <signal.h>
#include <sys/timerfd.h>
#include <poll.h>
#include <unistd.h>

#define LOCONET_INTERLOCK

#include "CabBus.h"
#include "loconet_buffer.h"
#include "loconet_print.h"

enum LocoRequestState{
    STATE_NONE,
    STATE_REQUEST,
    STATE_NULL_MOVE,
    STATE_ACK,
};

//
// Local Variables
//
static int loconet_timer_fd;
static c_serial_port_t* loconet_port;
static c_serial_port_t* cabbus_port;

//internal table to keep track of which slot has which locomotive
static uint16_t slot_table[ 128 ];

struct LoconetInfoForCab {
	uint8_t slot_number;
    enum LocoRequestState request_state;
    uint16_t request_loco_number;
};

static struct LoconetInfoForCab cab_info[ 64 ];
static LoconetContext* lnContext;
static struct CabBusContext* cabContext;

//
// Local Functions
//

//Loconet Functions


static void loconet_timer_start( uint32_t time ){
	static struct itimerspec timespec;

	memset( &timespec, 0, sizeof( struct itimerspec ) );
	timespec.it_value.tv_nsec = time * 1000; // microseconds to nanoseconds
	timespec.it_interval.tv_nsec = timespec.it_value.tv_nsec;

	if( timerfd_settime( loconet_timer_fd, 0, &timespec, NULL ) < 0 ){
		perror( "timer_settime" );
	}
}

static int loconet_read(){
    uint8_t buffer[ 20 ];
    int buffer_size = 20;
    ssize_t x;
    int status;

    status = c_serial_read_data( loconet_port, buffer, &buffer_size, NULL );
    if( status != CSERIAL_OK ){
        return -1;
    }

    printf( "LN read %d bytes\n", buffer_size );

    for( x = 0; x < buffer_size; x++ ){
        ln_incoming_byte( lnContext, buffer[ x ] );
    }

    return 0;
}

// Cabbus support functions
static void cabbus_delay( uint32_t delayms ){
	usleep( delayms * 10000 );
}


static int cabbus_read(){
	uint8_t buffer[ 20 ];
	int buffer_size = 20;
	ssize_t x;
	int status;

	status = c_serial_read_data( cabbus_port, buffer, &buffer_size, NULL );
	if( status != CSERIAL_OK ){
		return -1;
	}

	for( x = 0; x < buffer_size; x++ ){
        cabbus_incoming_byte( cabContext, buffer[ x ] );
	}

	return 0;
}

// Writing functions
static void loconet_write( uint8_t byte ){
	int length = sizeof( uint8_t );
	c_serial_write_data( loconet_port, &byte, &length );
}

static void cabbus_write( void* data, uint8_t len ){
	int length = len;
	c_serial_write_data( cabbus_port, data, &length );
	c_serial_flush( cabbus_port );
}

static void cserial_log_function( const char* logger_name,
        const struct SL_LogLocation* location,
	const enum SL_LogLevel level,
        const char* logMessage ){
	fprintf( stderr, "%s\n", logMessage );
}

//
// External Functions
//

int main( int argc, char** argv ){
	Ln_Message incomingMessage;
	Ln_Message outgoingMessage;
	struct Cab* cab;
    struct cab_command* cmd;
	int status;
	struct pollfd pollfds[ 3 ];
	c_serial_errnum_t errnum = 0;
	c_serial_port_t* tmpPort;
    int available;

	c_serial_set_global_log_function( cserial_log_function );

	//quick parse of cmdline
	if( argc < 3 ){
		fprintf( stderr, "ERROR: Need at least 2 args: <loconet port> <cabbus port>\n" );
		return 1;
	}

	//local variable setup
    memset( slot_table, 0, sizeof( slot_table ) );

	//set up the timer for loconet
	loconet_timer_fd = timerfd_create( CLOCK_REALTIME, 0 );

	//open the TTY ports
	printf( "About to open %s for loconet use\n", argv[ 1 ] );
	status = c_serial_new( &tmpPort, &errnum );
	loconet_port = tmpPort;
	c_serial_set_port_name( loconet_port, argv[ 1 ] );
    status = c_serial_open_keep_settings( loconet_port, 1 );
	if( status != CSERIAL_OK ){
		fprintf( stderr, "ERROR: Can't open loconet port: %s\n",
			c_serial_get_error_string( status ) );
		return 1;
	}
	
	printf( "About to open %s for cabbus use\n", argv[ 2 ] );
	status = c_serial_new( &cabbus_port, NULL );
	if( status < 0 ){
		fprintf( stderr, "ERROR: Can't allocate new serial port for cabbus\n" );
		return 1;
	}
	c_serial_set_port_name( cabbus_port, argv[ 2 ] );
	c_serial_set_stop_bits( cabbus_port, CSERIAL_STOP_BITS_2 );
	status = c_serial_open( cabbus_port );
	if( status != CSERIAL_OK ){
		fprintf( stderr, "ERROR: Can't open cabbus port: %s (%s)\n",
			c_serial_get_error_string( status ),
			strerror( c_serial_get_last_native_errnum( cabbus_port ) )  );
		return 1;
	}

	//initialize loconet
    lnContext = ln_context_new( loconet_timer_start, loconet_write );
    ln_context_set_ignore_state( lnContext, 1 );
    ln_context_set_additional_delay( lnContext, 200 );

	//initalize cabbus
    cabContext = cabbus_new( cabbus_delay, cabbus_write );

	// Ensure that each cab has a pointer to user data
	memset( cab_info, 0, sizeof( cab_info ) );
	for( int x = 0; x < 64; x++ ){
        struct Cab* cab = cabbus_cab_by_id( cabContext, x );
		if( !cab ) continue;
		cabbus_set_user_data( cab, &cab_info[ x ] );
	}

	//go into our main loop.
	//essentially what we do here, is we get information from the cabs on the bus,
	//and then echo that information back onto loconet.
	//we also have to parse loconet information that we get back to make sure
	//that we tell the user about stupid stuff that they are doing
	while( 1 ){
        cabbus_ping_step1( cabContext );
		c_serial_get_available( cabbus_port, &available );
		if( available ){
			cabbus_read();
		}
        cab = cabbus_ping_step2( cabContext );

		if( cab != NULL ){
//			printf( "got response from cab %d\n", cabbus_get_cab_number( cab ) );
			
			cmd = cabbus_get_command( cab );
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
                struct LoconetInfoForCab* info = cabbus_get_user_data( cab );
				outgoingMessage.opcode = LN_OPC_LOCO_ADDR;
                outgoingMessage.addr.locoAddrLo = cmd->sel_loco.address & 0x7F;
                outgoingMessage.addr.locoAddrHi = (cmd->sel_loco.address & ~0x7F) >> 7;
                info->request_state = STATE_REQUEST;
                info->request_loco_number = cmd->sel_loco.address;
                loconet_print_message( stdout, &outgoingMessage );
                if( ln_write_message( lnContext, &outgoingMessage ) < 0 ){
					fprintf( stderr, "ERROR writing outgoing message\n" );
				}
			}else if( cmd->command == CAB_CMD_SPEED ){
				struct LoconetInfoForCab* info = cabbus_get_user_data( cab );
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
					message.speed.speed  = 0;
				}
				message.speed.slot = info->slot_number;
				if( info->slot_number == 0 ){
					printf( "ERROR no slot number\n" );
				}
                printf( "Going to write Loconet message:\n" );
                loconet_print_message( stdout, &message );
                if( ln_write_message( lnContext, &message ) < 0 ){
					printf( "ERROR writing message\n" );
				}
            }else if( cmd->command == CAB_CMD_DIRECTION ||
                      cmd->command == CAB_CMD_FUNCTION ){
                struct LoconetInfoForCab* info = cabbus_get_user_data( cab );
                Ln_Message message;
                message.opcode = LN_OPC_LOCO_DIR_FUNC;
                message.dirFunc.slot = info->slot_number;
                message.dirFunc.dir_funcs = 0;
                if( cmd->direction.direction == CAB_DIR_REVERSE ){
                    LOCONET_SET_DIRECTION_REV(message);
                }else{
                    LOCONET_SET_DIRECTION_FWD(message);
                }

                // First set all of the known values that we have for our functions
                message.dirFunc.dir_funcs |= (cabbus_get_function( cab, 0 ) << 4);
                message.dirFunc.dir_funcs |= (cabbus_get_function( cab, 1 ) << 0);
                message.dirFunc.dir_funcs |= (cabbus_get_function( cab, 2 ) << 1);
                message.dirFunc.dir_funcs |= (cabbus_get_function( cab, 3 ) << 2);
                message.dirFunc.dir_funcs |= (cabbus_get_function( cab, 4 ) << 3);

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
                if( ln_write_message( lnContext, &message ) < 0 ){
                    printf( "ERROR writing message\n" );
                }
            }
		}

/*
		FD_ZERO( &set );
		FD_SET( STDIN_FILENO, &set );
		good = 0;
		do{
			ts.tv_sec = 0;
			ts.tv_usec = 10;
			if( select( 1, &set, NULL, NULL, &ts ) < 0 ){
				if( errno == EINTR ){
					continue;
				}
				perror( "select" );
			}
			good = 1;
		}while( !good );

		if( FD_ISSET( STDIN_FILENO, &set ) ){
			got = read( STDIN_FILENO, userCommand, 100 );
			if( got < 0 ){
				perror( "read - stdin" );
				break;
			}

			userCommand[ got ] = 0;
			printf( "User command: %s\n", userCommand );
			//quick and ugly parse
			if( memcmp( userCommand, "SEL", 3 ) == 0 ){
				selectingLoco = atoi( userCommand + 3 );
				outgoingMessage.opcode = LN_OPC_LOCO_ADDR;
				outgoingMessage.addr.locoAddrLo = selectingLoco & 0x7F;
				outgoingMessage.addr.locoAddrHi = (selectingLoco & ~0x7F) >> 7;
				selectingState = STATE_REQUEST;
				if( ln_write_message( &outgoingMessage ) < 0 ){
					fprintf( stderr, "ERROR writing outgoing message\n" );
				}
			}
		}
*/

        c_serial_get_available( loconet_port, &available );
        if( available ){
            loconet_read();
        }

        if( ln_read_message( lnContext, &incomingMessage ) == 1 ){
            printf( "Incoming loconet message:\n" );
			loconet_print_message( stdout, &incomingMessage );
			if( incomingMessage.opcode == LN_OPC_SLOT_READ_DATA ){
				//check to see if this loco addr is what we just requested
				int addr = incomingMessage.rdSlotData.addr1 | (incomingMessage.rdSlotData.addr2 << 7);
                struct LoconetInfoForCab* info;
                struct Cab* cab;
                for( int x = 0; x < ( sizeof( cab_info ) / sizeof( cab_info[ 0 ] ) ); x++ ){
                    cab = cabbus_cab_by_id( cabContext, x );
                    info = cabbus_get_user_data( cab );
                    if( info == NULL ){
                        continue;
                    }
                    if( info->request_state == STATE_REQUEST &&
                            info->request_loco_number == addr ){
                        //perform a NULL MOVE
                        outgoingMessage.opcode = LN_OPC_MOVE_SLOT;
                        outgoingMessage.moveSlot.source = incomingMessage.rdSlotData.slot;
                        outgoingMessage.moveSlot.slot = incomingMessage.rdSlotData.slot;
                        info->request_state = STATE_NULL_MOVE;
                        info->slot_number = incomingMessage.rdSlotData.slot;
                        if( ln_write_message( lnContext, &outgoingMessage ) < 0 ){
                            fprintf( stderr, "ERROR writing outgoing message\n" );
                        }
                        break;
                    }

                    if( info->request_state == STATE_NULL_MOVE &&
                            info->request_loco_number == addr ){
                        // We have successfully done the NULL MOVE! :D
                        // Now tell the cabbus that we have done so.
                        cabbus_set_loco_number( cab, info->request_loco_number );
                    }
                }
			}else if( incomingMessage.opcode == LN_OPC_LONG_ACK ){
                if( incomingMessage.ack.lopc & 0x7F == LN_OPC_MOVE_SLOT ){
                    printf( "Ack? %d\n", incomingMessage.ack.ack );
                }

                struct LoconetInfoForCab* info;
                for( int x = 0; x < ( sizeof( cab_info ) / sizeof( cab_info[ 0 ] ) ); x++ ){
                    info = cabbus_get_user_data( cabbus_cab_by_id( cabContext, x ) );
                    if( info == NULL ){
                        continue;
                    }
                    if( info->request_state == STATE_NULL_MOVE ){
                        info->request_state = STATE_NONE;
                    }
                }
            }else if( incomingMessage.opcode == LN_OPC_LOCO_SPEED ){
                struct LoconetInfoForCab* info;
                struct Cab* cab;
                for( int x = 0; x < ( sizeof( cab_info ) / sizeof( cab_info[ 0 ] ) ); x++ ){
                    cab = cabbus_cab_by_id( cabContext, x );
                    info = cabbus_get_user_data( cab );
                    if( info == NULL ){
                        continue;
                    }

                    if( info->slot_number == incomingMessage.speed.slot ){
                        int realSpeed = incomingMessage.speed.speed - 1;
                        if( realSpeed < 0 ){
                            realSpeed = 0;
                        }
                        cabbus_set_loco_speed( cab, realSpeed & 0xFF );
                    }
                }
            }else if( incomingMessage.opcode == LN_OPC_LOCO_DIR_FUNC ){
                struct LoconetInfoForCab* info;
                struct Cab* cab;
                for( int x = 0; x < ( sizeof( cab_info ) / sizeof( cab_info[ 0 ] ) ); x++ ){
                    cab = cabbus_cab_by_id( cabContext, x );
                    info = cabbus_get_user_data( cab );
                    if( info == NULL ){
                        continue;
                    }

                    if( info->slot_number == incomingMessage.dirFunc.slot ){
                        if( LOCONET_GET_DIRECTION_REV(incomingMessage) ){
                            cabbus_set_direction( cab, CAB_DIR_REVERSE );
                        }else{
                            cabbus_set_direction( cab, CAB_DIR_FORWARD );
                        }

                        char lights = !!(incomingMessage.dirFunc.dir_funcs & (0x01 << 4));
                        char f1 = !!(incomingMessage.dirFunc.dir_funcs & (0x01 << 0));
                        char f2 = !!(incomingMessage.dirFunc.dir_funcs & (0x01 << 1));
                        char f3 = !!(incomingMessage.dirFunc.dir_funcs & (0x01 << 2));
                        char f4 = !!(incomingMessage.dirFunc.dir_funcs & (0x01 << 3));
                        cabbus_set_functions( cab, 0, lights );
                        cabbus_set_functions( cab, 1, f1 );
                        cabbus_set_functions( cab, 2, f2 );
                        cabbus_set_functions( cab, 3, f3 );
                        cabbus_set_functions( cab, 4, f4 );

                    }
                }
            }
		}

        fflush( stdout );
        fflush( stderr );
		usleep( 1000 );
	}
}
