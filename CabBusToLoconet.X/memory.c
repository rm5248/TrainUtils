#include <plib.h>

#include "memory.h"

//the spot in memory where the locomotive address is stored.
#define STORAGE_LOCO_ADDR       0
//the address of the EEPROM on the I2C bus
#define MEMORY_BUS_ADDR         0x50

static BOOL transmitI2CByte(uint8_t byte) {
    // Wait for the transmitter to be ready
    while (!I2CTransmitterIsReady(I2C2));

    // Transmit the byte
    if (I2CSendByte(I2C2, byte) == I2C_MASTER_BUS_COLLISION) {
        return FALSE;
    }

    // Wait for the transmission to finish
    while (!I2CTransmissionHasCompleted(I2C2));

    return TRUE;
}

static BOOL StartTransfer( BOOL restart ) {
    I2C_STATUS status;

    if(restart)
    {
        I2CRepeatStart(I2C2);
    }
    else
    {
        // Wait for the bus to be idle, then start the transfer
        while (!I2CBusIsIdle(I2C2));

        if (I2CStart(I2C2) != I2C_SUCCESS) {
            return FALSE;
        }
    }

    // Wait for the signal to complete
    do {
        status = I2CGetStatus(I2C2);
    } while (!(status & I2C_START));

    return TRUE;
}

static void StopTransfer(void) {
    I2C_STATUS status;

    // Send the Stop signal
    I2CStop(I2C2);

    // Wait for the signal to complete
    do {
        status = I2CGetStatus(I2C2);

    } while (!(status & I2C_STOP));
}

void memory_store_loco_addr(uint16_t locoNumber) {
    I2C_7_BIT_ADDRESS storeAddr;
    uint8_t loco_storage[ 4 ];
    BOOL Acknowledged;
    
    I2C_FORMAT_7_BIT_ADDRESS(storeAddr, MEMORY_BUS_ADDR, I2C_WRITE);
    loco_storage[ 0 ] = storeAddr.byte;
    loco_storage[ 1 ] = STORAGE_LOCO_ADDR;
    loco_storage[ 2 ] = (locoNumber & 0xFF00) >> 8;
    loco_storage[ 3 ] = locoNumber & 0x00FF;

    StartTransfer( FALSE );

    transmitI2CByte(loco_storage[ 0 ]);
    while( !I2CByteWasAcknowledged( I2C2 ) );
    transmitI2CByte(loco_storage[ 1 ]);
    while( !I2CByteWasAcknowledged( I2C2 ) );
    transmitI2CByte(loco_storage[ 2 ]);
    while( !I2CByteWasAcknowledged( I2C2 ) );
    transmitI2CByte(loco_storage[ 3 ]);
    while( !I2CByteWasAcknowledged( I2C2 ) );

    StopTransfer();

    // Wait for EEPROM to complete write process, by polling the ack status.
    Acknowledged = FALSE;
    do {
        StartTransfer( FALSE );

        // Transmit just the EEPROM's address
        if (transmitI2CByte(storeAddr.byte)) {
            // Check to see if the byte was acknowledged
            Acknowledged = I2CByteWasAcknowledged(I2C2);
        }
        StopTransfer();

    } while (Acknowledged != TRUE);
}

uint16_t memory_get_loco_addr() {
    I2C_7_BIT_ADDRESS storeAddr;
    uint8_t loco_storage[ 2 ];
    I2C_STATUS status;
    BOOL Acknowledged;
    uint8_t firstByte;
    uint8_t secondByte;
    uint16_t returned;
    uint8_t times;

    I2C_FORMAT_7_BIT_ADDRESS(storeAddr, MEMORY_BUS_ADDR, I2C_WRITE);
    loco_storage[ 0 ] = storeAddr.byte;
    loco_storage[ 1 ] = STORAGE_LOCO_ADDR;

    StartTransfer( FALSE );

    //set the pointer address that is internal to the EEPROM
    transmitI2CByte( loco_storage[ 0 ] );
    while( !I2CByteWasAcknowledged( I2C2 ) );
    transmitI2CByte( loco_storage[ 1 ] );
    while( !I2CByteWasAcknowledged( I2C2 ) );

    //stop the transfer, this will set the address
    //StopTransfer();

    //wait a bit
    for( times = 1; times != 0; times++ ){
        _nop();
    }
//    Acknowledged = FALSE;
//    do {
//        StartTransfer( FALSE );
//
//        // Transmit just the EEPROM's address
//        if (transmitI2CByte(storeAddr.byte)) {
//            // Check to see if the byte was acknowledged
//            Acknowledged = I2CByteWasAcknowledged(I2C2);
//        }
//        StopTransfer();
//
//    } while (Acknowledged != TRUE);

    //Start a new transfer, reading from the EEPROM
    StartTransfer( TRUE );
    I2C_FORMAT_7_BIT_ADDRESS(storeAddr, MEMORY_BUS_ADDR, I2C_READ);
    transmitI2CByte( storeAddr.byte );
    while( !I2CByteWasAcknowledged( I2C2 ) );

    //Now read the next two addresses
    I2CReceiverEnable( I2C2, TRUE );
    while(!I2CReceivedDataIsAvailable(I2C2));
    I2CAcknowledgeByte( I2C2, TRUE );
    while( !I2CAcknowledgeHasCompleted( I2C2 ) );
    firstByte = I2CGetByte( I2C2 );

    I2CReceiverEnable( I2C2, TRUE );
    while(!I2CReceivedDataIsAvailable(I2C2));
    secondByte = I2CGetByte( I2C2 );
    StopTransfer();

    returned = (firstByte << 8) | secondByte;
    return returned;
}
