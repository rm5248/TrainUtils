#include <stdio.h>
#include <termios.h>
#include <fcntl.h>
#include <inttypes.h>

#include <errno.h>

#define ENTER 0x40
#define PROGRAM 0x41
#define RECALL 0x42
#define DIR_TOGGLE 0x43
#define SEL_LOCO 0x48
#define NO_KEY 0x7D

//
// Local Variables
//
int cabbus_fd;

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

int main( int argc, char** argv ){
	uint8_t bytes[ 16 ];
	int got;
	struct termios termio;
	int flags;
	uint8_t addr;

	if( argc < 2 ){
		fprintf( stderr, "ERROR: need cabbus TTY\n" );
		return 1;
	}

	cabbus_fd = open( argv[ 1 ], O_RDWR );
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

	flags = 0;
	while( 1 ){
		got = read( cabbus_fd, bytes, 1 );
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
