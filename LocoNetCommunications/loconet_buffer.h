/**
 * Get and parse Loconet information.
 * 
 * @Author Robert Middleton
 */
#ifndef LOCONET_BUFFER_H
#define LOCONET_BUFFER_H

#include <inttypes.h>

//
// Macro definitions
//
#ifdef LOCONET_INTERLOCK
  #define LN_WRITE_BYTE( byte ) \
		got_byte = 0;\
		lnLastTransmit = byte;\
		writeFunc( byte ); \
		while( !got_byte ){}
#else
  #define LN_WRITE_BYTE( byte )\
        lnLastTransmit = byte; \
		writeFunc( byte ); 
#endif

#define CHECK_BIT(number,bit)   (number & (0x1 << bit))
#define SET_BIT(number, bit)    (number |= (0x1 << bit))
#define CLEAR_BIT(number, bit)  (number &= (~(0x1 << bit)))

//
// Useful macros for setting various bits
//
#define LOCONET_SET_LOCO_ADDR(ln, addr) ln.addr.locoAddrHi = ((addr & 0x7F) << 7); \
    ln.addr.locoAddrLo = (addr & 0x7F);
//NOTE NOTE NOTE: the following two macros are REVERSED from what the
//loconet spec says that they should be.  The spec says that bit 5 set
//is forward, however the DT400 sends bit 5 set = reversed.
#define LOCONET_SET_DIRECTION_REV(ln) SET_BIT(ln.dirFunc.dir_funcs,5)
#define LOCONET_SET_DIRECTION_FWD(ln) CLEAR_BIT(ln.dirFunc.dir_funcs,5)
#define LOCONET_GET_DIRECTION_REV(ln) CHECK_BIT(ln.dirFunc.dir_funcs,5)

//
// Struct Definitions
//

// Command = 0xBF
typedef struct {
	uint8_t locoAddrHi;
	uint8_t locoAddrLo;
} Ln_Loco_Addr;

// Command = 0xBD
typedef struct {
	uint8_t switch1;
	uint8_t switch2;
} Ln_Switch_Ack;

// Command = 0xBC
typedef struct {
	uint8_t	switch1;
	uint8_t switch2;
} Ln_Switch_State;

// Command = 0xBB
typedef struct {
	uint8_t slot;
	uint8_t nul;
} Ln_Request_Slot_Data;

// Command = 0xBA
typedef struct {
	uint8_t source;
	uint8_t slot;
} Ln_Move_Slot;

// Command = 0xB9
typedef struct {
	uint8_t slot1;
	uint8_t slot2;
} Ln_Link_Slots;

// Command = 0xB8
typedef struct {
	uint8_t slot1;
	uint8_t slot2;
} Ln_Unlink_Slots;

// 0xBF - 0xB8 have responses

// Command = 0xB6
typedef struct {
	uint8_t slot;
	uint8_t direction;
} Ln_Consist;

// Command = 0xB5
typedef struct {
	uint8_t slot;
	uint8_t stat1;
} Ln_Write_Slot_Stat1;

// Command = 0xB4
typedef struct {
	uint8_t lopc; // Long opcode, i.e. OPCODE & 0x7F
	uint8_t ack; //ack status
} Ln_Ack;

// Command = 0xB2
typedef struct {
	uint8_t in1;
	uint8_t in2;
} Ln_Inputs;

// Command = 0xB1
typedef struct {
	uint8_t sn1;
	uint8_t sn2;
} Ln_Turnout;

// Command = 0xB0
typedef struct {
	uint8_t sw1;
	uint8_t sw2;
} Ln_Request_Switch;

// Command = 0xA2
typedef struct {
	uint8_t slot;
	uint8_t snd;
} Ln_Sound;

// Command = 0xA1
typedef struct {
	uint8_t slot;
	uint8_t dir_funcs;
} Ln_Direction_Funcs;

// Command = 0xA0
typedef struct {
	uint8_t slot;
	uint8_t speed;
} Ln_Speed;

// Command = 0xEF
typedef struct {
	uint8_t len;
	uint8_t slot;
	uint8_t stat;
	uint8_t addr1;
	uint8_t speed;
	uint8_t dir_funcs;
	uint8_t track;
	uint8_t stat2;
	uint8_t addr2;
	uint8_t sound;
	uint8_t id1;
	uint8_t id2;
} Ln_Write_Slot_Data, Ln_Read_Slot_Data;

typedef struct {
	uint8_t opcode;
	union{
		Ln_Loco_Addr 			addr;
		Ln_Switch_Ack 			switchAck;
		Ln_Switch_State 		switchState;
		Ln_Request_Slot_Data 	reqSlotData;
		Ln_Move_Slot 			moveSlot;
		Ln_Link_Slots			linkSlot;
		Ln_Unlink_Slots			unlinkSlot;
		Ln_Consist				consist;
		Ln_Write_Slot_Stat1		stat1;
		Ln_Ack					ack;
		Ln_Inputs				inputs;
		Ln_Turnout				turnout;
		Ln_Request_Switch		reqSwitch;
		Ln_Sound				sound;
		Ln_Direction_Funcs		dirFunc;
		Ln_Speed				speed;
		Ln_Write_Slot_Data		wrSlotData;
		Ln_Read_Slot_Data		rdSlotData;
		uint8_t data[ 16 ];
	};
} Ln_Message;
	

// What state the loconet currently is
typedef enum {
	LN_IDLE,
	LN_RX,
	LN_TX,
	LN_COLLISION,
	LN_CD_BACKOFF,
	LN_CD_BACKOFF_ADDITIONAL,
	LN_WAIT_MASTER
} Ln_State;

//
// Command defines
//
#define LN_OPC_IDLE 0x85
#define LN_OPC_POWER_ON 0x83
#define LN_OPC_POWER_OFF 0x82
#define LN_OPC_BUSY 0x81
#define LN_OPC_LOCO_ADDR 0xBF
#define LN_OPC_SWITCH_ACK 0xBD
#define LN_OPC_SWITCH_STATE 0xBC
#define LN_OPC_REQUEST_SLOT_DATA 0xBB
#define LN_OPC_MOVE_SLOT 0xBA
#define LN_OPC_LINK_SLOT 0xB9
#define LN_OPC_UNLINK_SLOT 0xB8
#define LN_OPC_CONSIST_FUNC 0xB6
#define LN_OPC_SLOT_STAT1 0xB5
#define LN_OPC_LONG_ACK 0xB4
#define LN_OPC_INPUT_REPORT 0xB2
#define LN_OPC_SWITCH_REPORT 0xB1
#define LN_OPC_SWITCH_REQUEST 0xB0
#define LN_OPC_LOCO_SOUND 0xA2
#define LN_OPC_LOCO_DIR_FUNC 0xA1
#define LN_OPC_LOCO_SPEED 0xA0
#define LN_OPC_SLOT_WRITE_DATA 0xEF
#define LN_OPC_SLOT_READ_DATA 0xE7

//
// Function typedefs
//

/**
 * This function is called to start a timer
 * The parameter is the number of microseconds to set the timer for
 * Note that this routine should modify whatever timer it creates,
 * instead of creating a new timer each time.  
 */
typedef void (*timerStartFn)( uint32_t );

/**
 * This function is called to actually write a byte out to the bus.
 * This function should wait until the byte has actually been written
 */
typedef void (*writeFn)( uint8_t );

//
// Function Definitions
//

/**
 * Initialize internal memory and internal pointers.
 *
 * @param timerStartFn A function which will start a timer
 * @param writeFn The function to call to write a byte out to the bus
 * @param additionalDelay How many more uS to wait before attempting network access
 */
void ln_init( timerStartFn, writeFn, uint8_t additinalDelay );

/**
 * Read the next available message from the bus.
 * See: ln_incoming_byte for mutex instructions
 *
 * @return 1 if the message is valid, 0 if no message,
 * -1 if the checksum was bad(data will be discarded)
 */
int ln_read_message( Ln_Message* );

/**
 * Write a message to the bus.  Will not return until the message
 * has been written OR it could not transmit the message
 *
 * @return 1 on success, 0 otherwise
 */
int ln_write_message( Ln_Message* );

/**
 * Get the state of loconet
 */
Ln_State ln_get_state();

/**
 * Call this when the timer fires
 */
void ln_timer_fired();

/**
 * Call this when a new byte comes in.
 * This routine may be called from an ISR.
 * If this routine is NOT called from an ISR,
 * you must properly mutex your calls to 
 * ln_incoming_byte and ln_read_message
 */
void ln_incoming_byte( uint8_t byte );

#endif
