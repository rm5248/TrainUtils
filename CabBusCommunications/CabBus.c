#include <string.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

#include "CabBus.h"
#include "Bitset.h"

#define CABBUS_SECRET
#include "cabbus_private.h"

#define CABBUS_NO_PING_TIMES 30

//
// Local Structs
//

struct cabbus_context {
    struct cabbus_cab allCabs[ 64 ];
    uint8_t currentCabAddr;
    cab_delay_fn delayFunction;
    cab_write_fn writeFunction;
    uint32_t ping_num;
    volatile uint8_t firstByte;
    volatile uint8_t secondByte;
    volatile uint8_t byteStatus;
    void* user_data;
};

//
// Local variables
//

//
// Local functions
//

//static void cabbus_send_current_control_info(struct cabbus_context* ctx, struct cabbus_cab* cab){
//    ctx->outputBuffer[ 0 ] = COMMAND_STATION_START_BYTE |
//        CURRENT_CONTROL_LOCOMOTIVE_INFO;
//    ctx->outputBuffer[ 1 ] = 0;
//    ctx->outputBuffer[ 2 ] = 0;
//    ctx->outputBuffer[ 3 ] = 0;
//    ctx->outputBuffer[ 4 ] = 0;

//    if( cab->speed & 0x80 ){
//        // Going forward
//        ctx->outputBuffer[ 1 ] |= (0x01 << 5);
//    }
//    // Upper 5 bits of speed in the first byte
//    ctx->outputBuffer[ 1 ] |= (cab->speed & 0x7C);

//    // Lower 2 bits of speed in bits 4,5 of second byte
//    ctx->outputBuffer[ 2 ] |= ((cab->speed & 0x03) << 4);
//    // Upper 4 bits of loco number come next
//    ctx->outputBuffer[ 2 ] |= ((cab->loco_number & 0xF000) >> 12 );
//    ctx->outputBuffer[ 3 ] |= ((cab->loco_number & 0x0F00) << 2);
//    ctx->outputBuffer[ 3 ] |= ((cab->loco_number & 0x00C0) >> 6);
//    ctx->outputBuffer[ 4 ] = (cab->loco_number & 0x003F);

//    ctx->writeFunction( ctx->outputBuffer, 5 );
//}

//
// Cabbus functions
//

struct cabbus_context* cabbus_new(cab_delay_fn inDelay, cab_write_fn inWrite) {
    unsigned char x;
    struct cabbus_context* ctx = malloc( sizeof( struct cabbus_context ) );

    memset( ctx, 0, sizeof( struct cabbus_context ) );

    ctx->delayFunction = inDelay;
    ctx->writeFunction = inWrite;
    ctx->ping_num = CABBUS_NO_PING_TIMES;

    for (x = 0; x < 64; x++) {
        ctx->allCabs[ x ].number = x;
        ctx->allCabs[ x ].new_cursor_position = -1;
        // Set defaults for our cabs; will also set the screens to be dirty
        cabbus_cab_set_loco_number(&ctx->allCabs[ x ], 44);
        cabbus_cab_set_loco_speed(&ctx->allCabs[ x ], 0);
        cabbus_cab_set_time(&ctx->allCabs[ x ], 5, 55, 1);
        cabbus_cab_set_functions(&ctx->allCabs[ x ], 1, 1);
        cabbus_cab_set_direction(&ctx->allCabs[ x ], CAB_DIR_FORWARD);
        ctx->allCabs[ x ].command.command = CAB_CMD_NONE;
        ctx->allCabs[ x ].dirty_screens = 0x0F; // All screens dirty
    }

    return ctx;
}

void cabbus_free( struct cabbus_context* ctx ){
    free( ctx );
}

static uint8_t cabbus_next_ping_address( struct cabbus_context* ctx ){
    uint8_t nextAddr = ctx->currentCabAddr;
    struct cabbus_cab* cab;
    int haveNextAddr = 0;

    do{
        nextAddr++;
        if( nextAddr == 1 || nextAddr == 2 ) nextAddr = 3;

        if( nextAddr == 64 ){
            // Always make sure to ping address 0 no matter what.
            nextAddr = 0;
            ctx->ping_num++;
            break;
        }

        cab = &ctx->allCabs[ nextAddr ];

        if( (ctx->ping_num - cab->last_ping) > CABBUS_NO_PING_TIMES ){
            // This cab has not been seen in at least CABBUS_NO_PING_TIMES pings.
            // Should we ping this guy, or wait just a bit?
            switch( (ctx->ping_num + cab->number) % 4 ){
            case 0:
                haveNextAddr = 1;
                break;
            }
        }else if( (ctx->ping_num - cab->last_ping) < CABBUS_NO_PING_TIMES ){
            // We have recently seen this guy - ping him again
            haveNextAddr = 1;
        }
    }while( !haveNextAddr );

    return nextAddr;
}

void cabbus_ping_step1( struct cabbus_context* ctx ){
    if( ctx == NULL ){
        return;
    }

    ctx->currentCabAddr = cabbus_next_ping_address( ctx );

    //Go and ping the next address
    uint8_t output_byte = 0x80 | ctx->currentCabAddr;

    ctx->writeFunction(&output_byte, 1);

    //Delay to make sure that we get a response back
    ctx->delayFunction(1);
}

struct cabbus_cab* cabbus_ping_step2( struct cabbus_context* ctx ){
    struct cabbus_cab* current = NULL;

    if( ctx == NULL ){
        return NULL;
    }

    if (ctx->ping_num - ctx->allCabs[ ctx->currentCabAddr ].last_ping > 1) {
        //we haven't seen this guy, set all screens to dirty
        ctx->allCabs[ ctx->currentCabAddr ].dirty_screens |= 0x0F;
    }

    if (ctx->byteStatus & 0x01) {
        //we have a response back from a cab
        unsigned char knobByte;
        unsigned char keyByte;
        unsigned int loopTimes = 0;

        current = &ctx->allCabs[ ctx->currentCabAddr ];
        current->command.command = CAB_CMD_NONE;

        loopTimes = 0;
        while (!(ctx->byteStatus & 0x02)) {
            ++loopTimes;
            if (loopTimes > 1000) {
                ctx->byteStatus = 0x00;
                return NULL;
            }
        }
        knobByte = ctx->secondByte;
        keyByte = ctx->firstByte;
        ctx->byteStatus = 0x00;

        current->last_ping = ctx->ping_num;

        if (keyByte != NO_KEY) {
            cabbus_cab_process_button_press( ctx->writeFunction, current, keyByte);
        }

        cabbus_cab_output_dirty_screens(ctx->writeFunction, current);
    }

    return current;
}

void cabbus_incoming_byte( struct cabbus_context* ctx, uint8_t byte) {
    if (ctx->byteStatus == 0x00) {
        //no bytes, put in first byte
        ctx->firstByte = byte;
        ctx->byteStatus = 0x01;
    } else if (ctx->byteStatus == 0x01) {
        ctx->secondByte = byte;
        ctx->byteStatus = 0x03;
    } else {
        ctx->byteStatus = 0x00;
    }
}

struct cabbus_cab* cabbus_cab_by_id( struct cabbus_context* ctx, int id ){
    if( id < 0 || id > (sizeof(ctx->allCabs) / sizeof(ctx->allCabs[0])) ){
		return NULL;
	}

    return &ctx->allCabs[ id ];
}

void cabbus_set_user_data( struct cabbus_context* ctx, void* user_data ){
    if( ctx == NULL ){
        return;
    }

    ctx->user_data = user_data;
}

void* cabbus_get_user_data( struct cabbus_context* ctx ){
    if( ctx == NULL ){
        return NULL;
    }
    return ctx->user_data;
}

void cabbus_set_all_cab_times( struct cabbus_context* ctx, int hours, int minutes, _Bool am ){
    int x;

    if( ctx == NULL ){
        return;
    }

    for (x = 0; x < 64; x++) {
        cabbus_cab_set_time( &ctx->allCabs[ x ], hours, minutes, am );
    }
}
