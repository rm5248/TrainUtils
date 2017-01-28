#include <string.h>
#include <stdio.h>

#include "loconet_buffer.h"


//
// Private variables
//
static timerStartFn timerStart;
static writeFn writeFunc;
static const uint8_t lnBufferLen = 128;
static uint8_t lnBuffer[ 128 ];
static volatile uint8_t lnBufferStart;
static volatile uint8_t lnBufferEnd;
static uint8_t tmpBuffer[ 16 ]; //a temporary loconet buffer
static uint8_t additionalDelay;
static volatile Ln_State currentState;
static volatile uint8_t lnLastTransmit;
static volatile uint8_t got_byte;

// 
// Function Implementations
//

// get how long the buffer currently is
static uint8_t get_ln_buffer_len(){
	if( lnBufferStart == lnBufferEnd ){
		return 0;
	}
	if( lnBufferStart < lnBufferEnd ){
		return lnBufferEnd - lnBufferStart;
	}

	return ( lnBufferLen - lnBufferStart ) + lnBufferEnd;
}

void ln_init( timerStartFn timStart, writeFn toWrite, uint8_t addDelay ){
	timerStart = timStart;
	lnBufferStart = 0;
	lnBufferEnd = 0;
	memset( lnBuffer, 0, sizeof( lnBuffer ) );
	additionalDelay = addDelay;
	writeFunc = toWrite;
	got_byte = 0;
}

// if we get a bad checksum, we discard bytes one at a time.
// this is because we will assume that we may get spurious bytes,
// and we should be able to sync up afterwards
int ln_read_message( Ln_Message* message ){
	uint8_t checksum;
	uint8_t messageLen;
	uint8_t workingByte;

	checksum = 0xFF; // Checksum start = 0xFF, or in the next byte(s)
	if( get_ln_buffer_len() < 2 ){
		return 0;
	}

	workingByte = lnBuffer[ lnBufferStart ];
	workingByte = workingByte & 0xE0;
	if( workingByte == 0x80 ){
		// Two bytes, including checksum
		if( get_ln_buffer_len() < 2 ){
			return 0;
		}else{
			message->data[ 0 ] = lnBuffer[ lnBufferStart++ ];
			if( lnBufferStart >= lnBufferLen ){
				lnBufferStart = 0;
			}
			message->data[ 1 ] = lnBuffer[ lnBufferStart++ ];
			if( lnBufferStart >= lnBufferLen ){
				lnBufferStart = 0;
			}
			checksum ^= message->data[ 0 ];
			if( checksum != message->data[ 1 ] ){
				// checksum did not match, return an error and discard just the first byte
				lnBufferStart--;
				return -1;
			}
			return 1;
		}
	}else if( workingByte == 0xA0 ){
		// Four bytes, including checksum
		if( get_ln_buffer_len() < 4 ){
			return 0;
		}else{
			uint8_t msgLoc;
			uint8_t bufStart = lnBufferStart;
			if( lnBufferStart + 4 > lnBufferLen ){
				// this wraps.
				message->data[ 0 ] = lnBuffer[ bufStart++ ];
				if( bufStart >= lnBufferLen ){
					bufStart = 0;
				}
				message->data[ 1 ] = lnBuffer[ bufStart++ ];
				if( bufStart >= lnBufferLen ){
					bufStart = 0;
				}
				message->data[ 2 ] = lnBuffer[ bufStart++ ];
				if( bufStart >= lnBufferLen ){
					bufStart = 0;
				}
				message->data[ 3 ] = lnBuffer[ bufStart++ ];
				if( bufStart >= lnBufferLen ){
					bufStart = 0;
				}
			}else{
				memcpy( message, lnBuffer + lnBufferStart, 4 );
				bufStart += 4;
			}

			// calculate the checksum
			checksum ^= message->opcode;
			for( msgLoc = 0; msgLoc < 2; msgLoc++ ){
				checksum ^= message->data[ msgLoc ];
			}
			if( checksum != message->data[ 2 ] ){
				//checksum did not match, discard first byte
				lnBufferStart++;
				if( lnBufferStart >= lnBufferLen ){
					lnBufferStart = 0;
				}
				return -1;
			}else{
				lnBufferStart = bufStart;
				return 1;
			}
		}
	}else if( workingByte == 0xC0 ){
printf( "six byte\n" );
		// six bytes, including checksum
		if( get_ln_buffer_len() < 6 ){
			return 0;
		}else{
			uint8_t msgLoc;
			uint8_t bufStart = lnBufferStart;
			if( lnBufferStart + 6 > lnBufferLen ){
				// this wraps
				for( msgLoc = 0; msgLoc < 6; msgLoc++ ){
					message->data[ msgLoc ] = lnBuffer[ bufStart++ ];
					if( bufStart >= lnBufferLen ){
						bufStart = 0;
					}
				}
			}else{
				memcpy( message, lnBuffer + lnBufferStart, 6 );
				bufStart += 6;
			}

			// calculate the checksum
			checksum ^= message->opcode;
			for( msgLoc = 0; msgLoc < 4; msgLoc++ ){
				checksum ^= message->data[ msgLoc ];
			}
			if( checksum != message->data[ 4 ] ){
				//checksum did not match, discard first byte
				lnBufferStart++;
				if( lnBufferStart >= lnBufferLen ){
					lnBufferStart = 0;
				}
				return -1;
			}else{
				lnBufferStart = bufStart;
				return 1;
			}
		}
	}else if( workingByte == 0xE0 ){
		// N byte message, the next byte is the opcode length
		if( get_ln_buffer_len() < 2 ){
			return 0;
		}else{
			uint8_t msgLoc = 0;
			uint8_t checksum = 0xFF;
			uint8_t bufStart = lnBufferStart;

			//copy the opcode first
			message->opcode = lnBuffer[ bufStart++ ];
			if( bufStart >= lnBufferLen ){
				bufStart = 0;
			}
			//copy how many bytes long this message is
			message->data[ msgLoc++ ] = lnBuffer[ bufStart++ ];
			if( bufStart >= lnBufferLen ){
				bufStart = 0;
			}
			if( message->opcode == LN_OPC_SLOT_WRITE_DATA ||
				message->opcode == LN_OPC_SLOT_READ_DATA ){
				//apparently somebody at digitrax is an idiot and the len of these messages
				//doesn't actually correspond to how many bytes there are.
				//these are 14-byte messages.
				//we've already read two bytes, read the next 12.
				if( get_ln_buffer_len() < 12 ){
					return 0;
				}

				if( lnBufferStart + 12 > lnBufferLen ){
					memcpy( message->data + 1, lnBuffer + bufStart, 12 );
					bufStart += 12;
				}else{
					for( msgLoc = 1; msgLoc < 13; msgLoc++ ){
						message->data[ msgLoc ] = lnBuffer[ bufStart++ ];
						if( bufStart >= lnBufferLen ){
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
				lnBufferStart = bufStart;
				return 1;
			}

			printf( "Generic Variable length message, IMPLEMENT THIS\n" );
			return 0;
		}
	}else{
		// this must be bad data, discard it
		lnBufferStart++;
		if( lnBufferStart >= lnBufferLen ){
			lnBufferStart = 0;
		}
printf( "got spurious data\n" );

		return 0;
	}
}

int ln_write_message( Ln_Message* message ){
	//first, let's calculate our checksum
	uint8_t type = message->opcode & 0xE0;
	uint8_t checksum = 0xFF;

	while( currentState != LN_IDLE ){}

	currentState = LN_TX;

	checksum ^= message->opcode;
	if( type == 0x80 ){
		//two bytes, including checksum

		got_byte = 0;
		lnLastTransmit = message->opcode;
		writeFunc( message->opcode );
		while( !got_byte ){}

		got_byte = 0;
		lnLastTransmit = checksum;
		writeFunc( checksum );
		while( !got_byte );
	}else if( type == 0xA0 ){
		//four bytes, including checksum
		checksum ^= message->data[ 0 ];
		checksum ^= message->data[ 1 ];
		
		LN_WRITE_BYTE( message->opcode );
		LN_WRITE_BYTE( message->data[ 0 ] );
		LN_WRITE_BYTE( message->data[ 1 ] );
		LN_WRITE_BYTE( checksum );
	}

	currentState = LN_CD_BACKOFF;
	timerStart( 1200 );

	return 1;
}

Ln_State ln_get_state(){
	return currentState;
}

void ln_timer_fired(){
	if( currentState == LN_CD_BACKOFF ){
		//add in the aditional delay
		currentState = LN_CD_BACKOFF_ADDITIONAL;
		timerStart( additionalDelay );
	}else if( currentState == LN_CD_BACKOFF_ADDITIONAL ){
		currentState = LN_IDLE;
	}else if( currentState == LN_COLLISION ){
		currentState = LN_IDLE;
	}else if( currentState == LN_RX ){
		currentState = LN_IDLE;
	}
}

void ln_incoming_byte( uint8_t byte ){
	if( currentState == LN_IDLE ){
		currentState = LN_RX;
	}else if( currentState == LN_TX ){
		got_byte = 1;
		if( byte != lnLastTransmit ){
			//OH SNAP!
			//we have a collision on the bus
printf( "OH SNAP collision rx 0x%X tx 0x%X\n", byte, lnLastTransmit );
			currentState = LN_COLLISION;
			timerStart( 1000 ); // wait at least 15 bit times
			return;
		}

		//return; //let's not parse our own messages
	}

	lnBuffer[ lnBufferEnd ] = byte;
	lnBufferEnd++;
	if( lnBufferEnd >= lnBufferLen ){
		lnBufferEnd = 0;
	}

	timerStart( 360 ); // everything must wait AT LEAST 360 uS before attempting to transmit

/*
	// This is technicaly an error, but we don't have a good way of 
	// showing errors
	if( lnBufferEnd == lnBufferStart ){
		//discard data.
		lnBufferStart++;
	}
*/

}
