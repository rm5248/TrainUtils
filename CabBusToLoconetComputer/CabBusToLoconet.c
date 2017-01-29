#include <stdio.h>
#include <string.h>
#include <cserial/c_serial.h>

//linux includes here
#include <signal.h>
#include <sys/timerfd.h>
#include <poll.h>

#define LOCONET_INTERLOCK

#include "CabBus.h"
#include "loconet_buffer.h"
#include "loconet_print.h"

#define STATE_NONE 0
#define STATE_REQUEST 1
#define STATE_NULL_MOVE 2
#define STATE_ACK 3

//
// Local Variables
//
static int loconet_timer_fd;
static c_serial_port_t* loconet_port;
static c_serial_port_t* cabbus_port;

//internal table to keep track of which slot has which locomotive
static uint16_t slot_table[ 128 ];

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
		cabbus_incoming_byte( buffer[ x ] );
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
}

//
// External Functions
//

int main( int argc, char** argv ){
	Ln_Message incomingMessage;
	Ln_Message outgoingMessage;
	struct Cab* cab;
	struct cab_command* cmd;
	char userCommand[ 100 ];
	int got;
	int selectingLoco;
	int selectingState;
	int selectingSlot;
	int good;
	int status;
	struct pollfd pollfds[ 3 ];

	//quick parse of cmdline
	if( argc < 3 ){
		fprintf( stderr, "ERROR: Need at least 2 args: <loconet port> <cabbus port>\n" );
		return 1;
	}

	//local variable setup
	memset( slot_table, 0, sizeof( slot_table ) );
	selectingState = STATE_NONE;

	//set up the timer for loconet
	loconet_timer_fd = timerfd_create( CLOCK_REALTIME, 0 );

	//open the TTY ports
	printf( "About to open %s for loconet use\n", argv[ 1 ] );
	c_serial_new( &loconet_port, NULL );
	c_serial_set_port_name( loconet_port, argv[ 1 ] );
        status = c_serial_open( loconet_port );
	if( status != CSERIAL_OK ){
		fprintf( stderr, "ERROR: Can't open loconet port\n" );
		return 1;
	}
	
	printf( "About to open %s for cabbus use\n", argv[ 2 ] );
	c_serial_new( &cabbus_port, NULL );
	c_serial_set_port_name( cabbus_port, argv[ 2 ] );
	c_serial_set_stop_bits( cabbus_port, CSERIAL_STOP_BITS_2 );
	status = c_serial_open( cabbus_port );
	if( status != CSERIAL_OK ){
		fprintf( stderr, "ERROR: Can't open cabbus port\n" );
		return 1;
	}

	//initialize loconet
	ln_init( loconet_timer_start, loconet_write, 200 );

	//initalize cabbus
	cabbus_init( cabbus_delay, cabbus_write, NULL );


	//go into our main loop.
	//essentially what we do here, is we get information from the cabs on the bus,
	//and then echo that information back onto loconet.
	//we also have to parse loconet information that we get back to make sure
	//that we tell the user about stupid stuff that they are doing
	while( 1 ){
		cab = cabbus_ping_next();
		if( cab != NULL ){
			printf( "got response from cab %d\n", cabbus_get_cab_number( cab ) );
			
			cmd = cabbus_get_command( cab );
			if( cmd->command != CAB_CMD_NONE ){
				printf( "Got command %d\n", cmd->command );
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


		if( ln_read_message( &incomingMessage ) == 1 ){
			loconet_print_message( stdout, &incomingMessage );
			if( incomingMessage.opcode == LN_OPC_SLOT_READ_DATA ){
				//check to see if this loco addr is what we just requested
				int addr = incomingMessage.rdSlotData.addr1 | (incomingMessage.rdSlotData.addr2 << 7);
				if( addr == selectingLoco && selectingState == STATE_REQUEST ){
					//perform a NULL MOVE
					outgoingMessage.opcode = LN_OPC_MOVE_SLOT;
					outgoingMessage.moveSlot.source = incomingMessage.rdSlotData.slot;
					outgoingMessage.moveSlot.slot = incomingMessage.rdSlotData.slot;
					selectingState = STATE_NULL_MOVE;
					if( ln_write_message( &outgoingMessage ) < 0 ){
						fprintf( stderr, "ERROR writing outgoing message\n" );
					}
				}
			}else if( incomingMessage.opcode == LN_OPC_LONG_ACK ){
				if( incomingMessage.ack.lopc & 0x7F == LN_OPC_MOVE_SLOT ){
					printf( "Ack? %d\n", incomingMessage.ack.ack );
				}
			}
		}

		usleep( 1000 );
	}
}
