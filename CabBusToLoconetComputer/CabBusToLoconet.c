#include <stdio.h>
#include <time.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <termios.h>
#include <fcntl.h>
#include <sys/select.h>
#include <sys/types.h>
#include <unistd.h>

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
static volatile int loconet_fd;
static timer_t loconet_timer;

static volatile int cabbus_fd;

//internal table to keep track of which slot has which locomotive
static uint16_t slot_table[ 128 ];

//
// Local Functions
//

//Loconet Functions


static void timerStart( uint32_t time ){
	static struct itimerspec timespec;

	memset( &timespec, 0, sizeof( struct itimerspec ) );
	timespec.it_value.tv_nsec = time * 1000; // microseconds to nanoseconds
	timespec.it_interval.tv_nsec = timespec.it_value.tv_nsec;

	if( timer_settime( loconet_timer, 0, &timespec, NULL ) < 0 ){
		perror( "timer_settime" );
	}
}

static void timerFired( int sig, siginfo_t* si, void* uc ){
	static struct itimerspec timespec;

	memset( &timespec, 0, sizeof( struct itimerspec ) );
	//disable the timer
	timer_settime( loconet_timer, 0, &timespec, NULL );

	ln_timer_fired();
}

// Cabbus support functions
static void cabDelay( uint32_t delayms ){
	usleep( delayms * 10000 );
}

// Thread Functions

static void* loconet_read_thread( void* ign ){
	uint8_t buffer[ 20 ];
	ssize_t got;
	ssize_t x;

	while( 1 ){
		got = read( loconet_fd, buffer, 20 );
		if( got < 0 ){
			perror( "read - loconet" );
			break;
		}
		for( x = 0; x < got; x++ ){
			ln_incoming_byte( buffer[ x ] );
		}
	}

	return NULL;
}

static void* cabbus_read_thread( void* ign ){
	uint8_t buffer[ 20 ];
	ssize_t got;
	ssize_t x;

	while( 1 ){
		got = read( cabbus_fd, buffer, 20 );
		if( got < 0 ){
			perror( "read - cabbus" );
			break;
		}
		for( x = 0; x < got; x++ ){
//printf( "cab bus incoming 0x%X\n", buffer[ x ] );
			cabbus_incoming_byte( buffer[ x ] );
		}
	}

	return NULL;
}

// Writing functions
static void loconet_write( uint8_t byte ){
	printf( "TX byte 0x%X\n", byte );
fflush(stdout);
	if( write( loconet_fd, &byte, 1 ) < 0 ){
		perror( "write - loconet" );
	}
}

static void cabbus_write( void* data, uint8_t len ){
	if( write( cabbus_fd, data, len ) < 0 ){
		perror( "write - cabbus" );
	}
}

//
// External Functions
//

int main( int argc, char** argv ){
	pthread_t ln_thr;
	pthread_t cab_thr;
	struct sigevent evt;
	struct sigaction sa;
	sigset_t mask;
	Ln_Message incomingMessage;
	Ln_Message outgoingMessage;
	struct Cab* cab;
	struct termios termio;
	struct cab_command* cmd;
	char userCommand[ 100 ];
	fd_set set;
	int got;
	struct timeval ts;
	int selectingLoco;
	int selectingState;
	int selectingSlot;
	int good;

	//quick parse of cmdline
	if( argc < 3 ){
		fprintf( stderr, "ERROR: Need at least 2 args: <loconet port> <cabbus port>\n" );
		return 1;
	}

	//local variable setup
	memset( slot_table, 0, sizeof( slot_table ) );
	selectingState = STATE_NONE;

	//set up the timer for loconet
	memset( &sa, 0, sizeof( struct sigaction ) );
	sa.sa_flags = SA_SIGINFO;
	sa.sa_sigaction = timerFired;
	sigemptyset( &sa.sa_mask );
	if( sigaction( SIGRTMIN, &sa, NULL ) < 0 ){
		perror( "sigaction" );
		return 1;
	}

	memset( &evt, 0, sizeof( evt ) );
	evt.sigev_notify = SIGEV_SIGNAL;
	evt.sigev_signo = SIGRTMIN;
	evt.sigev_value.sival_ptr = &loconet_timer;
	if( timer_create( CLOCK_REALTIME, &evt, &loconet_timer ) < 0 ){
		perror( "timer_create" );
		return 1;
	}

	//open the TTY ports
	printf( "About to open %s for loconet use\n", argv[ 1 ] );
	loconet_fd = open( argv[ 1 ], O_RDWR );
	if( loconet_fd < 0 ){
		fprintf( stderr, "ERROR: Can't open.  Reason: %s\n", strerror( errno ) );
		return 1;
	}
	
	printf( "About to open %s for cabbus use\n", argv[ 2 ] );
	cabbus_fd = open( argv[ 2 ], O_RDWR );
	if( cabbus_fd < 0 ){
		fprintf( stderr, "ERROR: Can't open.  Reason: %s\n", strerror( errno ) );
		return 1;
	}

	//set up the cab bus port
	if( tcgetattr( cabbus_fd, &termio ) < 0 ){
		perror( "tcgetattr" );
	}
	cfsetospeed( &termio, B9600 );
	cfsetispeed( &termio, B9600 );
	termio.c_iflag |= IGNBRK;
	termio.c_iflag &= ~BRKINT;
	termio.c_iflag &= ~ICRNL;
	termio.c_oflag = 0;
	termio.c_lflag = 0;
	termio.c_cc[VTIME] = 0;
	termio.c_cc[VMIN] = 1;
	termio.c_cflag |= CS8;
	termio.c_cflag |= CSTOPB;
	termio.c_iflag &= ~( PARODD | PARENB );
	termio.c_iflag |= IGNPAR;
	termio.c_iflag &= ~( IXON | IXOFF | IXANY );
	termio.c_cflag &= ~CRTSCTS;
	if( tcsetattr( cabbus_fd, TCSANOW, &termio ) < 0 ){
		perror( "tcsetattr" );
	}

	//initialize loconet
	ln_init( timerStart, loconet_write, 200 );

	//initalize cabbus
	cabbus_init( cabDelay, cabbus_write, NULL );

	//start our threads
	if( pthread_create( &ln_thr, NULL, loconet_read_thread, NULL ) < 0 ){
		perror( "pthread_create - loconet" );
		return 1;
	}

	if( pthread_create( &cab_thr, NULL, cabbus_read_thread, NULL ) < 0 ){
		perror( "pthread_create - cabbus" );
	}

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
