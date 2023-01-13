#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "loconet_buffer.h"

#define LN_BUFFER_LEN 128

#ifdef LOCONET_INTERLOCK
  #define LN_WRITE_BYTE( byte ) \
        got_byte = 0;\
        lnLastTransmit = byte;\
        writeFunc( byte ); \
        while( !got_byte ){}
#else
  #define LN_WRITE_BYTE( ctx, byte )\
        ctx->lnLastTransmit = byte; \
        ctx->writeFunc( ctx, byte );
#endif

//
// Private variables
//
struct loconet_context {
    lnTimerStartFn timerStart;
    lnWriteFn writeFunc;
    lnWriteInterlockFn writeInterlock;
    uint8_t additionalDelay;
    volatile enum loconet_state currentState;
    uint8_t lnBufferLocation;
    uint8_t lnBuffer[ LN_BUFFER_LEN ];
    int ignoreState;
    struct loconet_time fastclockTime;
    volatile uint8_t lnLastTransmit;
    volatile uint8_t got_byte;
    void* user_data;
};

// 
// Function Implementations
//

static void ln_update_current_time( struct loconet_context* ctx, struct loconet_message* message ){
    if( message->slot_data.slot != 123 ){
        return;
    }

    int hours = ((0xFF - message->clock_slot_data.hours_24) & 0x7F) % 24;
    hours = (24 - hours) % 24;
    int minutes = ((0xFF - message->clock_slot_data.mins_60) & 0x7F) % 60;
    minutes = (60 - minutes) % 60;

    ctx->fastclockTime.hours = hours - 1;
    ctx->fastclockTime.minutes = minutes;
}

// get how long the buffer currently is
static uint8_t get_ln_buffer_len( struct loconet_context* ctx ){
    return ctx->lnBufferLocation;
}

// remove bytes from the front of our buffer, moving everything else down
static void ln_remove_bytes( struct loconet_context* ctx, int numBytes ){
    if( numBytes > ctx->lnBufferLocation ){
        ctx->lnBufferLocation = 0;
        return;
    }

    memmove( ctx->lnBuffer,
             ctx->lnBuffer + numBytes,
             ctx->lnBufferLocation - numBytes );
    ctx->lnBufferLocation -= numBytes;
}

struct loconet_context* ln_context_new( lnTimerStartFn timerStart, lnWriteFn write ){
    struct loconet_context* newContext = malloc( sizeof( struct loconet_context ) );
    memset( newContext, 0, sizeof( struct loconet_context ) );

    newContext->timerStart = timerStart;
    newContext->writeFunc = write;

    return newContext;
}

struct loconet_context* ln_context_new_interlocked( lnWriteInterlockFn writeInterlock ){
    struct loconet_context* newContext = malloc( sizeof( struct loconet_context ) );
    memset( newContext, 0, sizeof( struct loconet_context ) );

    newContext->writeInterlock = writeInterlock;

    return newContext;
}

void ln_context_set_additional_delay( struct loconet_context* context, uint8_t additionalDelay ){
    if( context == NULL ){
        return;
    }

    context->additionalDelay = additionalDelay;
}

void ln_context_set_ignore_state( struct loconet_context* ctx, int ignore_state ){
    if( ctx == NULL ){
        return;
    }

    ctx->ignoreState = ignore_state;
}

// if we get a bad checksum, we discard bytes one at a time.
// this is because we will assume that we may get spurious bytes,
// and we should be able to sync up afterwards
int ln_read_message( struct loconet_context* ctx, struct loconet_message* message ){
	uint8_t checksum;
	uint8_t messageLen;
	uint8_t workingByte;

    if( ctx == NULL ){
        return -2;
    }

	checksum = 0xFF; // Checksum start = 0xFF, or in the next byte(s)
    if( get_ln_buffer_len( ctx ) < 2 ){
		return 0;
	}

    printf( "Current buffer len: %d\n", get_ln_buffer_len( ctx ) );

    workingByte = ctx->lnBuffer[ 0 ];
	workingByte = workingByte & 0xE0;
	if( workingByte == 0x80 ){
		// Two bytes, including checksum
        if( get_ln_buffer_len( ctx ) < 2 ){
			return 0;
		}else{
            message->data[ 0 ] = ctx->lnBuffer[ 0 ];
            message->data[ 1 ] = ctx->lnBuffer[ 1 ];
			checksum ^= message->data[ 0 ];
			if( checksum != message->data[ 1 ] ){
				// checksum did not match, return an error and discard just the first byte
                ln_remove_bytes( ctx, 1 );
				return -1;
			}
            ln_remove_bytes( ctx, 2 );
			return 1;
		}
	}else if( workingByte == 0xA0 ){
		// Four bytes, including checksum
        if( get_ln_buffer_len( ctx ) < 4 ){
			return 0;
		}else{
            uint8_t msgLoc;
            memcpy( message, ctx->lnBuffer, 4 );
			// calculate the checksum
			checksum ^= message->opcode;
			for( msgLoc = 0; msgLoc < 2; msgLoc++ ){
				checksum ^= message->data[ msgLoc ];
			}
			if( checksum != message->data[ 2 ] ){
				//checksum did not match, discard first byte
                ln_remove_bytes( ctx, 1 );
				return -1;
			}else{
                ln_remove_bytes( ctx, 4 );
				return 1;
			}
		}
	}else if( workingByte == 0xC0 ){
printf( "six byte\n" );
		// six bytes, including checksum
        if( get_ln_buffer_len( ctx ) < 6 ){
			return 0;
        }else{
            uint8_t msgLoc;
            memcpy( message, ctx->lnBuffer, 6 );

			// calculate the checksum
			checksum ^= message->opcode;
            for( msgLoc = 0; msgLoc < 6; msgLoc++ ){
				checksum ^= message->data[ msgLoc ];
			}
            if( checksum != message->data[ 6 ] ){
				//checksum did not match, discard first byte
                ln_remove_bytes( ctx, 1 );
				return -1;
			}else{
                ln_remove_bytes( ctx, 6 );
				return 1;
			}
		}
	}else if( workingByte == 0xE0 ){
		// N byte message, the next byte is the opcode length
        if( get_ln_buffer_len( ctx ) < 2 ){
			return 0;
		}else{
			uint8_t msgLoc = 0;
            uint8_t checksum = 0xFF;

			//copy the opcode first
            message->opcode = ctx->lnBuffer[ 0 ];
			//copy how many bytes long this message is
            message->data[ msgLoc++ ] = ctx->lnBuffer[ 1 ];
			if( message->opcode == LN_OPC_SLOT_WRITE_DATA ||
				message->opcode == LN_OPC_SLOT_READ_DATA ){
				//apparently somebody at digitrax is an idiot and the len of these messages
				//doesn't actually correspond to how many bytes there are.
				//these are 14-byte messages.
				//we've already read two bytes, read the next 12.
                if( get_ln_buffer_len( ctx ) < 12 ){
					return 0;
				}

                memcpy( message->data + 1, ctx->lnBuffer + 2, 12 );

				checksum ^= message->opcode;
				for( msgLoc = 0; msgLoc < 12; msgLoc++ ){
					checksum ^= message->data[ msgLoc ];
				}
				if( checksum != message->data[ 12 ] ){
                    //checksum did not match, discard first byte
                    ln_remove_bytes( ctx, 1 );
					return -1;
				}
                ln_remove_bytes( ctx, 14 );

                if( message->opcode == LN_OPC_SLOT_READ_DATA &&
                        message->slot_data.slot == 123 ){
                    // Special case: this is the time
                    ln_update_current_time( ctx, message );
                }

				return 1;
            }else{
                uint8_t numBytes = ctx->lnBuffer[ 1 ];

                if( get_ln_buffer_len( ctx ) < numBytes ){
                    return 0;
                }

                memcpy( message->data + 1, ctx->lnBuffer + 2, numBytes );

                checksum ^= message->opcode;
                for( msgLoc = 0; msgLoc < numBytes; msgLoc++ ){
                    checksum ^= message->data[ msgLoc ];
                }
                if( checksum != message->data[ numBytes ] ){
                    //checksum did not match, discard first byte
                    ln_remove_bytes( ctx, 1 );
                    return -1;
                }
                ln_remove_bytes( ctx, numBytes );

                return 1;
            }

			return 0;
		}
	}else{
		// this must be bad data, discard it
        ln_remove_bytes( ctx, 1 );
printf( "got spurious data\n" );
fflush(stdout);

		return 0;
	}
}

int ln_write_message( struct loconet_context* ctx, struct loconet_message* message ){
	//first, let's calculate our checksum
	uint8_t type = message->opcode & 0xE0;
	uint8_t checksum = 0xFF;
    uint8_t out_data[12];

    if( !ctx->ignoreState ){
        while( ctx->currentState != LN_IDLE ){}
    }

    ctx->currentState = LN_TX;

	checksum ^= message->opcode;
	if( type == 0x80 ){
		//two bytes, including checksum
        if( ctx->writeInterlock ){
            out_data[0] = message->opcode;
            out_data[1] = checksum;
            ctx->writeInterlock( ctx, out_data, 2 );
            ctx->currentState = LN_IDLE;
        }else{
            ctx->got_byte = 0;
            ctx->lnLastTransmit = message->opcode;
            ctx->writeFunc( ctx, message->opcode );
            while( !ctx->got_byte ){}

            ctx->got_byte = 0;
            ctx->lnLastTransmit = checksum;
            ctx->writeFunc( ctx, checksum );
            while( !ctx->got_byte );
        }
    }else if( type == 0xA0 ){
        //four bytes, including checksum
        checksum ^= message->data[ 0 ];
        checksum ^= message->data[ 1 ];

        if( ctx->writeInterlock ){
            out_data[0] = message->opcode;
            out_data[1] = message->data[ 0 ];
            out_data[2] = message->data[ 1 ];
            out_data[3] = checksum;
            ctx->writeInterlock( ctx, out_data, 4 );
            ctx->currentState = LN_IDLE;
        }else{
            LN_WRITE_BYTE( ctx, message->opcode );
            LN_WRITE_BYTE( ctx, message->data[ 0 ] );
            LN_WRITE_BYTE( ctx, message->data[ 1 ] );
            LN_WRITE_BYTE( ctx, checksum );
        }
	}

    if( ctx->timerStart ){
        ctx->currentState = LN_CD_BACKOFF;
        ctx->timerStart( ctx, 1200 );
    }

	return 1;
}

enum loconet_state ln_get_state( struct loconet_context* ctx ){
    return ctx->currentState;
}

void ln_timer_fired( struct loconet_context* ctx ){
    if( ctx->currentState == LN_CD_BACKOFF ){
		//add in the aditional delay
        ctx->currentState = LN_CD_BACKOFF_ADDITIONAL;
        ctx->timerStart( ctx, ctx->additionalDelay );
    }else if( ctx->currentState == LN_CD_BACKOFF_ADDITIONAL ){
        ctx->currentState = LN_IDLE;
    }else if( ctx->currentState == LN_COLLISION ){
        ctx->currentState = LN_IDLE;
    }else if( ctx->currentState == LN_RX ){
        ctx->currentState = LN_IDLE;
	}
}

void ln_incoming_byte( struct loconet_context* ctx, uint8_t byte ){
    if( ctx->currentState == LN_IDLE ){
        ctx->currentState = LN_RX;
    }else if( ctx->currentState == LN_TX ){
        ctx->got_byte = 1;
        if( byte != ctx->lnLastTransmit ){
			//OH SNAP!
			//we have a collision on the bus
printf( "OH SNAP collision rx 0x%X tx 0x%X\n", byte, ctx->lnLastTransmit );
            ctx->currentState = LN_COLLISION;
            ctx->timerStart( ctx, 1000 ); // wait at least 15 bit times
			return;
		}

		//return; //let's not parse our own messages
	}

    ctx->lnBuffer[ ctx->lnBufferLocation ] = byte;
    ctx->lnBufferLocation++;
    if( ctx->lnBufferLocation >= LN_BUFFER_LEN ){
        ctx->lnBufferLocation = 0;
	}

    if( ctx->timerStart ){
        // everything must wait AT LEAST 360 uS before attempting to transmit
        // Not important if we are interlocked
        ctx->timerStart( ctx, 360 );
    }
/*
	// This is technicaly an error, but we don't have a good way of 
	// showing errors
	if( lnBufferEnd == lnBufferStart ){
		//discard data.
		lnBufferStart++;
	}
*/

}

struct loconet_time ln_current_time( struct loconet_context* ctx ){
    return ctx->fastclockTime;
}

void* ln_user_data( struct loconet_context* ctx ){
    if( ctx == NULL ) return NULL;
    return ctx->user_data;
}

void ln_set_user_data( struct loconet_context* ctx, void* user_data ){
    if( ctx == NULL ) return;
    ctx->user_data = user_data;
}
