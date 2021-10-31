#include <stdio.h>
#include <cserial/c_serial.h>
#include <string.h>
#include <time.h>

//linux includes here
#include <popt.h>
#include <signal.h>
#include <sys/timerfd.h>
#include <poll.h>

#include "loconet_buffer.h"
#include "loconet_print.h"

//
// Local variables
//
static int loconet_timer_fd;
static struct loconet_context* lnContext;

//
// Method Implementations
//

static void timerStart( uint32_t time ){
	static struct itimerspec timespec;

	memset( &timespec, 0, sizeof( struct itimerspec ) );
	timespec.it_value.tv_nsec = time * 1000;
	timespec.it_interval.tv_nsec = timespec.it_value.tv_nsec;
	if( timerfd_settime( loconet_timer_fd, 0, &timespec, NULL ) < 0 ){
		perror( "timer_settime" );
	}
}

static int loconet_read( c_serial_port_t* loconet_port ){
	uint8_t buffer[ 20 ];
	int buffer_size = 20;
	ssize_t x;
	int status;

	status = c_serial_read_data( loconet_port, buffer, &buffer_size, NULL );
	if( status != CSERIAL_OK ){
		return -1;
	}

	for( x = 0; x < buffer_size; x++ ){
        ln_incoming_byte( lnContext, buffer[ x ] );
	}

	return 0;
}

//stub, don't do anything
static void loconet_write( uint8_t ign ){}

int main( int argc, const char** argv ){
	FILE* textOutput;
	FILE* binaryOutput;
	int doBinaryOutput;
    struct loconet_message incomingMessage;
	char* serialPort;
	char* textFile;
	char* binaryFile;
	poptContext pc;
	int poptVal;
	pthread_t readThread;
	struct sigaction sa;
	struct sigevent evt;
	c_serial_port_t* loconet_port;
	int status;
	struct pollfd pollfds[ 2 ];

	struct poptOption options[] = {
		{ "serial-port", 's', POPT_ARG_STRING, &serialPort, 0, "The serial port that is connected to Loconet" },
		{ "text-file", 't', POPT_ARG_STRING, &textFile, 0, "The file to write text output to(default: stdout)" },
		{ "hex-out", 'b', POPT_ARG_NONE, &doBinaryOutput, 0, "Output binary data as formatted hex" },
		{ "hex-file", 'h', POPT_ARG_STRING, &binaryFile, 0, "Output binary data as formatted hex to the given file(default: stdout)"},
		POPT_AUTOHELP
		POPT_TABLEEND
	};

	textOutput = stdout;
	binaryOutput = stdout;
	doBinaryOutput = 0;
	serialPort = NULL;
	textFile = NULL;
	binaryFile = NULL;

	pc = poptGetContext( NULL, argc, argv, options, 0 );

	while( ( poptVal = poptGetNextOpt( pc ) ) >= 0 ){
		switch( poptVal ){
			case 'h': doBinaryOutput = 1; break;
		}
	}

	if( poptVal != -1 ){
		fprintf( stderr, "arg parsing error: %s\n", poptBadOption( pc, 0 ) );
		poptPrintHelp( pc, stderr, 0 );
		return poptVal;
	}

	if( serialPort == NULL ){
		fprintf( stderr, "ERROR: Need serial port to open\n" );
		return 1;
	}

	//first see if these have to point to the same file
	if( textFile != NULL && binaryFile != NULL ){
		if( strcmp( textFile, binaryFile ) == 0 ){
			//yes, these point to the same file
			textOutput = fopen( textFile, "w" );
			binaryOutput = textOutput;
		}else{
			textOutput = fopen( textFile, "w" );
			binaryOutput = fopen( binaryFile, "w" );
		}

		//error check, make sure that the files opened fine
		if( textOutput == NULL || binaryOutput == NULL ){
			fprintf( stderr, "ERROR: Unable to open up either text file or binary file\n" );
			return 1;
		}
	}else if( textFile != NULL ){
		textOutput = fopen( textFile, "w" );
		if( textOutput == NULL ){
			fprintf( stderr, "ERROR: Unable to open up text file\n" );
			return 1;
		}
	}else if( binaryFile != NULL ){
		binaryOutput = fopen( binaryFile, "w" );
		if( binaryOutput == NULL ){
			fprintf( stderr, "ERROR: Unable to open up binary file\n" );
			return 1;
		}
	}

	c_serial_new( &loconet_port, NULL );
	c_serial_set_port_name( loconet_port, serialPort );
    c_serial_set_baud_rate( loconet_port, CSERIAL_BAUD_57600 );
    status = c_serial_open( loconet_port );
	if( status != CSERIAL_OK ){
		fprintf( stderr, "ERROR: Unable to open serial port: %s\n",
			c_serial_get_error_string( status ) );
		return 1;
	}

	//loconet timer setup
	loconet_timer_fd = timerfd_create( CLOCK_REALTIME, 0 );

	//initialize loconet
    lnContext = ln_context_new( timerStart, loconet_write );
    ln_context_set_ignore_state( lnContext, 1 );
    ln_context_set_additional_delay( lnContext, 200 );

	//setup the FDs to poll
	pollfds[ 0 ].fd = loconet_timer_fd;
	pollfds[ 1 ].fd = c_serial_get_poll_handle( loconet_port );

	while( 1 ){
		int x;
		for( x = 0; x < 2; x++ ){
			pollfds[ x ].events = POLLIN;
			pollfds[ x ].revents = 0;
		}

		status = poll( pollfds, 2, -1 );
		if( status < 0 ){
			break;
		}

		if( pollfds[ 0 ].revents & POLLIN ){
			//the timer has expired
            ln_timer_fired( lnContext );
		}

		if( pollfds[ 1 ].revents & POLLIN ){
			//we have data on the serial port
			loconet_read( loconet_port );
		}

        if( ln_read_message( lnContext, &incomingMessage ) == 1 ){
			if( doBinaryOutput ){
				loconet_print_message_hex( binaryOutput, &incomingMessage );
			}
			loconet_print_message( textOutput, &incomingMessage );
        }
		fflush( textOutput );
		fflush( binaryOutput );
	}
}
