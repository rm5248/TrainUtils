#include <stdio.h>
#include <inttypes.h>
#include <cserial/c_serial.h>

#define ENTER 0x40
#define PROGRAM 0x41
#define RECALL 0x42
#define DIR_TOGGLE 0x43
#define SEL_LOCO 0x48
#define NO_KEY 0x7D

//
// Local Variables
//

//
// Local Functions
//

static const char* getKeyCode( uint8_t byte ){
	if( byte == ENTER ){
		return "ENTER";
	}

	if( byte == PROGRAM ){
		return "PROGRAM";
	}

	if( byte == RECALL ){
		return "RECALL";
	}

	if( byte == DIR_TOGGLE ){
		return "DIRECTION TOGGLE";
	}

	if( byte == SEL_LOCO ){
		return "SELECT LOCO";
	}

	if( byte == NO_KEY ){
		return "NO KEY";
	}

	return "UKN";
}

static void cserial_log_function( enum CSerial_Log_Level logLevel,
	const char* logMessage,
	const char* fileName,
	int lineNumber,
	const char* functionName,
	c_serial_port_t* port ){
	fprintf( stderr, "%s\n", logMessage );
}

static void print_error( c_serial_errnum_t errnum ){
#ifdef _WIN32
	LPVOID lpMsgBuf;
	DWORD dw = errnum;

	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		dw,
		MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ),
		(LPTSTR)&lpMsgBuf,
		0, NULL );
	fprintf( stderr, "ERROR: %s\n", lpMsgBuf );
#else
	fprintf( stderr, strerror( errnum ) );
#endif
}

int main( int argc, char** argv ){
	uint8_t bytes[ 16 ];
	int got;
	int flags;
	uint8_t addr;
	c_serial_port_t* port;
	int status;

	if( argc < 2 ){
		fprintf( stderr, "ERROR: need cabbus COM port\n" );
		return 1;
	}

	c_serial_set_global_log_function( cserial_log_function );

	if( c_serial_new( &port, NULL ) != CSERIAL_OK ){
		fprintf( stderr, "ERROR: Unable to open serial port\n" );
		return 1;
	}

	c_serial_set_port_name( port, argv[ 1 ] );
	c_serial_set_stop_bits( port, CSERIAL_STOP_BITS_2 );
	status = c_serial_open( port );

	if( status != CSERIAL_OK ){
		fprintf( stderr, "CSerial error: %s\n", c_serial_get_error_string( status ) );
		print_error( c_serial_get_last_native_errnum( port ) );
		return 1;
	}

	flags = 0;
	while( 1 ){
		int errnum;
		c_serial_errnum_t native_errnum;

		got = 1;
		errnum = c_serial_read_data( port, bytes, &got, NULL );
		if( errnum != CSERIAL_OK ){
			fprintf( stderr, "Did not read data properly: %s\n", c_serial_get_error_string( errnum ) );
			print_error( c_serial_get_last_native_errnum( port ) );
			break;
		}
		if( got < 0 ){
			fprintf( stderr, "ERROR: Unable to read: %s\n", strerror( errno ) );
			return 1;
		}
		
		if( ( bytes[ 0 ] & 0xC0 ) == 0x80 ){
			printf( "Ping address %d\n", bytes[ 0 ] & 0x3F );
			addr = bytes[ 0 ] & 0x3F;
		}else {
//if( addr == 10 ){
			if( flags == 0 ){
				//this must be the first byte, which is the key
				flags = 1;
			    printf( "  Data: 0x%02X(%s)\n", bytes[ 0 ], getKeyCode( bytes[ 0 ] ) );
			}else if( flags == 1 ){
				//this must be the second byte, speed
				flags = 0;
			    printf( "  Data: 0x%02X(Speed: %d)\n", bytes[ 0 ], bytes[ 0 ] & 0x7F );
			}
//}
		}
	}
}
