#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "loconet_buffer.h"

#define LN_BUFFER_LEN 128

//
// Private variables
//
struct loconet_context {
    timerStartFn timerStart;
    writeFn writeFunc;
    uint8_t additionalDelay;
    volatile enum loconet_state currentState;
    uint8_t lnBufferStart;
    uint8_t lnBufferEnd;
    uint8_t lnBuffer[ LN_BUFFER_LEN ];
    int ignoreState;
    volatile uint8_t lnLastTransmit;
    volatile uint8_t got_byte;
};

// 
// Function Implementations
//

// get how long the buffer currently is
static uint8_t get_ln_buffer_len( struct loconet_context* ctx ){
    if( ctx->lnBufferStart == ctx->lnBufferEnd ){
		return 0;
	}
    if( ctx->lnBufferStart < ctx->lnBufferEnd ){
        return ctx->lnBufferEnd - ctx->lnBufferStart;
	}

    return ( sizeof(ctx->lnBuffer) - ctx->lnBufferStart ) + ctx->lnBufferEnd;
}

struct loconet_context* ln_context_new( timerStartFn timerStart, writeFn write ){
    struct loconet_context* newContext = malloc( sizeof( struct loconet_context ) );
    memset( newContext, 0, sizeof( struct loconet_context ) );

    newContext->timerStart = timerStart;
    newContext->writeFunc = write;

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

    workingByte = ctx->lnBuffer[ ctx->lnBufferStart ];
	workingByte = workingByte & 0xE0;
	if( workingByte == 0x80 ){
		// Two bytes, including checksum
        if( get_ln_buffer_len( ctx ) < 2 ){
			return 0;
		}else{
            message->data[ 0 ] = ctx->lnBuffer[ ctx->lnBufferStart++ ];
            if( ctx->lnBufferStart >= LN_BUFFER_LEN ){
                ctx->lnBufferStart = 0;
			}
            message->data[ 1 ] = ctx->lnBuffer[ ctx->lnBufferStart++ ];
            if( ctx->lnBufferStart >= LN_BUFFER_LEN ){
                ctx->lnBufferStart = 0;
			}
			checksum ^= message->data[ 0 ];
			if( checksum != message->data[ 1 ] ){
				// checksum did not match, return an error and discard just the first byte
                ctx->lnBufferStart--;
				return -1;
			}
			return 1;
		}
	}else if( workingByte == 0xA0 ){
		// Four bytes, including checksum
        if( get_ln_buffer_len( ctx ) < 4 ){
			return 0;
		}else{
			uint8_t msgLoc;
            uint8_t bufStart = ctx->lnBufferStart;
            if( ctx->lnBufferStart + 4 > LN_BUFFER_LEN ){
				// this wraps.
                message->data[ 0 ] = ctx->lnBuffer[ bufStart++ ];
                if( bufStart >= LN_BUFFER_LEN ){
					bufStart = 0;
				}
                message->data[ 1 ] = ctx->lnBuffer[ bufStart++ ];
                if( bufStart >= LN_BUFFER_LEN ){
					bufStart = 0;
				}
                message->data[ 2 ] = ctx->lnBuffer[ bufStart++ ];
                if( bufStart >= LN_BUFFER_LEN ){
					bufStart = 0;
				}
                message->data[ 3 ] = ctx->lnBuffer[ bufStart++ ];
                if( bufStart >= LN_BUFFER_LEN ){
					bufStart = 0;
				}
			}else{
                memcpy( message, ctx->lnBuffer + ctx->lnBufferStart, 4 );
				bufStart += 4;
			}

			// calculate the checksum
			checksum ^= message->opcode;
			for( msgLoc = 0; msgLoc < 2; msgLoc++ ){
				checksum ^= message->data[ msgLoc ];
			}
			if( checksum != message->data[ 2 ] ){
				//checksum did not match, discard first byte
                ctx->lnBufferStart++;
                if( ctx->lnBufferStart >= LN_BUFFER_LEN ){
                    ctx->lnBufferStart = 0;
				}
				return -1;
			}else{
                ctx->lnBufferStart = bufStart;
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
            uint8_t bufStart = ctx->lnBufferStart;
            if( ctx->lnBufferStart + 6 > LN_BUFFER_LEN ){
				// this wraps
				for( msgLoc = 0; msgLoc < 6; msgLoc++ ){
                    message->data[ msgLoc ] = ctx->lnBuffer[ bufStart++ ];
                    if( bufStart >= LN_BUFFER_LEN ){
						bufStart = 0;
					}
				}
			}else{
                memcpy( message, ctx->lnBuffer + ctx->lnBufferStart, 6 );
				bufStart += 6;
			}

			// calculate the checksum
			checksum ^= message->opcode;
			for( msgLoc = 0; msgLoc < 4; msgLoc++ ){
				checksum ^= message->data[ msgLoc ];
			}
			if( checksum != message->data[ 4 ] ){
				//checksum did not match, discard first byte
                ctx->lnBufferStart++;
                if( ctx->lnBufferStart >= LN_BUFFER_LEN ){
                    ctx->lnBufferStart = 0;
				}
				return -1;
			}else{
                ctx->lnBufferStart = bufStart;
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
            uint8_t bufStart = ctx->lnBufferStart;

			//copy the opcode first
            message->opcode = ctx->lnBuffer[ bufStart++ ];
            if( bufStart >= LN_BUFFER_LEN ){
				bufStart = 0;
			}
			//copy how many bytes long this message is
            message->data[ msgLoc++ ] = ctx->lnBuffer[ bufStart++ ];
            if( bufStart >= LN_BUFFER_LEN ){
				bufStart = 0;
			}
			if( message->opcode == LN_OPC_SLOT_WRITE_DATA ||
				message->opcode == LN_OPC_SLOT_READ_DATA ){
				//apparently somebody at digitrax is an idiot and the len of these messages
				//doesn't actually correspond to how many bytes there are.
				//these are 14-byte messages.
				//we've already read two bytes, read the next 12.
                if( get_ln_buffer_len( ctx ) < 12 ){
					return 0;
				}

                if( ctx->lnBufferStart + 12 > LN_BUFFER_LEN ){
                    memcpy( message->data + 1, ctx->lnBuffer + bufStart, 12 );
					bufStart += 12;
				}else{
					for( msgLoc = 1; msgLoc < 13; msgLoc++ ){
                        message->data[ msgLoc ] = ctx->lnBuffer[ bufStart++ ];
                        if( bufStart >= LN_BUFFER_LEN ){
							bufStart = 0;
						}
					}
				}

				checksum ^= message->opcode;
				for( msgLoc = 0; msgLoc < 12; msgLoc++ ){
					checksum ^= message->data[ msgLoc ];
				}
				if( checksum != message->data[ 12 ] ){
					return -1;
				}
                ctx->lnBufferStart = bufStart;
				return 1;
			}

			printf( "Generic Variable length message, IMPLEMENT THIS\n" );
			return 0;
		}
	}else{
		// this must be bad data, discard it
        ctx->lnBufferStart++;
        if( ctx->lnBufferStart >= LN_BUFFER_LEN ){
            ctx->lnBufferStart = 0;
		}
printf( "got spurious data\n" );

		return 0;
	}
}

int ln_write_message( struct loconet_context* ctx, struct loconet_message* message ){
	//first, let's calculate our checksum
	uint8_t type = message->opcode & 0xE0;
	uint8_t checksum = 0xFF;

    if( !ctx->ignoreState ){
        while( ctx->currentState != LN_IDLE ){}
    }

    ctx->currentState = LN_TX;

	checksum ^= message->opcode;
	if( type == 0x80 ){
		//two bytes, including checksum

        ctx->got_byte = 0;
        ctx->lnLastTransmit = message->opcode;
        ctx->writeFunc( message->opcode );
        while( !ctx->got_byte ){}

        ctx->got_byte = 0;
        ctx->lnLastTransmit = checksum;
        ctx->writeFunc( checksum );
        while( !ctx->got_byte );
	}else if( type == 0xA0 ){
		//four bytes, including checksum
		checksum ^= message->data[ 0 ];
		checksum ^= message->data[ 1 ];
		
        LN_WRITE_BYTE( ctx, message->opcode );
        LN_WRITE_BYTE( ctx, message->data[ 0 ] );
        LN_WRITE_BYTE( ctx, message->data[ 1 ] );
        LN_WRITE_BYTE( ctx, checksum );
	}

    ctx->currentState = LN_CD_BACKOFF;
    ctx->timerStart( 1200 );

	return 1;
}

enum loconet_state ln_get_state( struct loconet_context* ctx ){
    return ctx->currentState;
}

void ln_timer_fired( struct loconet_context* ctx ){
    if( ctx->currentState == LN_CD_BACKOFF ){
		//add in the aditional delay
        ctx->currentState = LN_CD_BACKOFF_ADDITIONAL;
        ctx->timerStart( ctx->additionalDelay );
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
            ctx->timerStart( 1000 ); // wait at least 15 bit times
			return;
		}

		//return; //let's not parse our own messages
	}

    ctx->lnBuffer[ ctx->lnBufferEnd ] = byte;
    ctx->lnBufferEnd++;
    if( ctx->lnBufferEnd >= LN_BUFFER_LEN ){
        ctx->lnBufferEnd = 0;
	}

    ctx->timerStart( 360 ); // everything must wait AT LEAST 360 uS before attempting to transmit

/*
	// This is technicaly an error, but we don't have a good way of 
	// showing errors
	if( lnBufferEnd == lnBufferStart ){
		//discard data.
		lnBufferStart++;
	}
*/

}
