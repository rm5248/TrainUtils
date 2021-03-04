#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/timerfd.h>
#include <poll.h>
#include <unistd.h>

#include <cserial/c_serial.h>

#define LOCONET_INTERLOCK

#include "CabBus.h"
#include "loconet_buffer.h"
#include "loconet_print.h"
#include "cabbus_to_loconet.h"

static c_serial_port_t* loconet_port;
static c_serial_port_t* cabbus_port;
static struct loconet_context* loconet_context;
static struct cabbus_context* cab_context;

//Loconet Functions


static void loconet_timer_start( uint32_t time ){
//    static struct itimerspec timespec;

//    memset( &timespec, 0, sizeof( struct itimerspec ) );
//    timespec.it_value.tv_nsec = time * 1000; // microseconds to nanoseconds
//    timespec.it_interval.tv_nsec = timespec.it_value.tv_nsec;

//    if( timerfd_settime( loconet_timer_fd, 0, &timespec, NULL ) < 0 ){
//        perror( "timer_settime" );
//    }
}

static int loconet_read(void* ignore){
    uint8_t buffer[ 20 ];
    int buffer_size = 20;
    ssize_t x;
    int status;
    int avail;

    if( c_serial_get_available( loconet_port, &avail ) < 0 ){
        return 0;
    }

    if( avail <= 0 ){
        return 0;
    }

    status = c_serial_read_data( loconet_port, buffer, &buffer_size, NULL );
    if( status != CSERIAL_OK ){
        return -1;
    }

    for( x = 0; x < buffer_size; x++ ){
        ln_incoming_byte( loconet_context, buffer[ x ] );
    }

    return 0;
}

// Cabbus support functions
static void cabbus_delay( uint32_t delayms ){
    usleep( delayms * 10000 );
}


static int cabbus_read(void* ignore){
    uint8_t buffer[ 20 ];
    int buffer_size = 20;
    ssize_t x;
    int status;
    int avail;

    if( c_serial_get_available( cabbus_port, &avail ) < 0 ){
        return 0;
    }

    if( avail <= 0 ){
        return 0;
    }

    status = c_serial_read_data( cabbus_port, buffer, &buffer_size, NULL );
    if( status != CSERIAL_OK ){
        return -1;
    }

    for( x = 0; x < buffer_size; x++ ){
        cabbus_incoming_byte( cab_context, buffer[ x ] );
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


int main( int argc, char** argv ){
    int status;
    int errnum;

    c_serial_set_global_log_function( cserial_log_function );

    //open the TTY ports
    printf( "About to open %s for loconet use\n", argv[ 1 ] );
    status = c_serial_new( &loconet_port, &errnum );
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
    loconet_context = ln_context_new( loconet_timer_start, loconet_write );
    ln_context_set_ignore_state( loconet_context, 1 );
    ln_context_set_additional_delay( loconet_context, 200 );

    //initalize cabbus
    cab_context = cabbus_new( cabbus_delay, cabbus_write );

    cabbus_to_loconet_main(cab_context,
                           cabbus_write,
                           cabbus_read,
                           NULL,
                           loconet_context,
                           loconet_read,
                           NULL);
}
