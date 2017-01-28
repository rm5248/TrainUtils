#include <stdio.h>
#include <unistd.h>
#include <popt.h>
#include <fcntl.h>
#include <popt.h>
#include <pthread.h>
#include <string.h>
#include <time.h>
#include <signal.h>

#include "loconet_buffer.h"
#include "loconet_print.h"

//
// Local variables
//
static int loconet_fd;
static timer_t loconet_timer;

//
// Method Implementations
//

static void timerStart( uint32_t time ){
	static struct itimerspec timespec;

	memset( &timespec, 0, sizeof( struct itimerspec ) );
	timespec.it_value.tv_nsec = time * 1000;
	timespec.it_interval.tv_nsec = timespec.it_value.tv_nsec;
	if( timer_settime( loconet_timer, 0, &timespec, NULL ) < 0 ){
		perror( "timer_settime" );
	}
}

static void timerFired( int sig, siginfo_t* si, void* uc ){
	static struct itimerspec timespec;

	memset( &timespec, 0, sizeof( struct itimerspec ) );
	timer_settime( loconet_timer, 0, &timespec, NULL );

	ln_timer_fired();
}

static void* loconet_read( void* ign ){
	uint8_t buffer[ 20 ];
	ssize_t got;
	ssize_t x;

	while( 1 ){
		got = read( loconet_fd, buffer, 20 );
		if( got < 0 ){
			perror( "read - loconet" );
			return NULL;
		}

		for( x = 0; x < got; x++ ){
			ln_incoming_byte( buffer[ x ] );
		}
	}

	return NULL;
}

//stub, don't do anything
static void loconet_write( uint8_t ign ){}

int main( int argc, const char** argv ){
	FILE* textOutput;
	FILE* binaryOutput;
	int doBinaryOutput;
	Ln_Message incomingMessage;
	char* serialPort;
	char* textFile;
	char* binaryFile;
	poptContext pc;
	int poptVal;
	pthread_t readThread;
	struct sigaction sa;
	struct sigevent evt;

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
			perror( "fopen" );
			return 1;
		}
	}else if( textFile != NULL ){
		textOutput = fopen( textFile, "w" );
		if( textOutput == NULL ){
			perror( "fopen" );
			return 1;
		}
	}else if( binaryFile != NULL ){
		binaryOutput = fopen( binaryFile, "w" );
		if( binaryOutput == NULL ){
			perror( "fopen" );
			return 1;
		}
	}

	loconet_fd = open( serialPort, O_RDWR );
	if( loconet_fd < 0 ){
		perror( "open" );
		return 2;
	}

	//loconet timer setup
	memset( &sa, 0, sizeof( struct sigaction ) );
	sa.sa_flags = SA_SIGINFO;
	sa.sa_sigaction = timerFired,
	sigemptyset( &sa.sa_mask );
	if( sigaction( SIGRTMIN, &sa, NULL ) < 0 ){
		perror( "sigaction" );
		return 2;
	}

	memset( &evt, 0, sizeof( struct sigevent ) );
	evt.sigev_notify = SIGEV_SIGNAL;
	evt.sigev_signo = SIGRTMIN;
	evt.sigev_value.sival_ptr = &loconet_timer;
	if( timer_create( CLOCK_REALTIME, &evt, &loconet_timer ) < 0 ){
		perror( "timer_create" );
		return 2;
	}

	//initialize loconet
	ln_init( timerStart, loconet_write, 200 );

	//Create a read thread to read the data from loconet
	if( pthread_create( &readThread, NULL, loconet_read, NULL ) < 0 ){
		perror( "pthread_create" );
		return 4;
	}

	
	while( 1 ){
		if( ln_read_message( &incomingMessage ) == 1 ){
			if( doBinaryOutput ){
				loconet_print_message_hex( binaryOutput, &incomingMessage );
			}
			loconet_print_message( textOutput, &incomingMessage );
		}	
		fflush( textOutput );
		fflush( binaryOutput );
	}
}
