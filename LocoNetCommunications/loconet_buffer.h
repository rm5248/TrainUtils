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
  #define LN_WRITE_BYTE( ctx, byte )\
        ctx->lnLastTransmit = byte; \
        ctx->writeFunc( byte );
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
#define LOCONET_SET_DIRECTION_REV(byte) SET_BIT(byte,5)
#define LOCONET_SET_DIRECTION_FWD(byte) CLEAR_BIT(byte,5)
#define LOCONET_GET_DIRECTION_REV(byte) CHECK_BIT(byte,5)

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

enum Ln_SlotStatus{
    LN_SLOT_STATUS_FREE,
    LN_SLOT_STATUS_COMMON,
    LN_SLOT_STATUS_IDLE,
    LN_SLOT_STATUS_IN_USE,
};

typedef struct LoconetContext LoconetContext;

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

#define LN_SLOT_STATUS(LnMessage) (LnMessage.rdSlotData.stat & ( 0x03 << 4 ) >> 4)

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
 * Initialize a new LoconetContext
 *
 * @param timerStartFn A function which will start a timer
 * @param writeFn The function to call to write a byte out to the bus
 * @param additionalDelay How many more uS to wait before attempting network access
 */
LoconetContext* ln_context_new( timerStartFn start, writeFn write );

void ln_context_free( LoconetContext* context );

void ln_context_set_additional_delay( LoconetContext* ctx, uint8_t additionalDelay );

/**
 * Set if we should ignore the state or not.
 *
 * If you are running on a system where you are directly connected to the Loconet bus,
 * don't set this.  Ignoring the state is fine if there is already a device which will
 * handle the backoff stuff(e.g. a PR3)
 *
 * @param ctx
 * @param ignore_state
 */
void ln_context_set_ignore_state( LoconetContext* ctx, int ignore_state );

/**
 * Read the next available message from the bus.
 * See: ln_incoming_byte for mutex instructions
 *
 * @return 1 if the message is valid, 0 if no message,
 * -1 if the checksum was bad(data will be discarded)
 */
int ln_read_message( LoconetContext* ctx, Ln_Message* );

/**
 * Write a message to the bus.  Will not return until the message
 * has been written OR it could not transmit the message
 *
 * @return 1 on success, 0 otherwise
 */
int ln_write_message( LoconetContext* ctx, Ln_Message* );

/**
 * Get the state of loconet
 */
Ln_State ln_get_state( LoconetContext* ctx );

/**
 * Call this when the timer fires
 */
void ln_timer_fired( LoconetContext* ctx );

/**
 * Call this when a new byte comes in.
 * This routine may be called from an ISR.
 * If this routine is NOT called from an ISR,
 * you must properly mutex your calls to 
 * ln_incoming_byte and ln_read_message
 */
void ln_incoming_byte( LoconetContext* ctx, uint8_t byte );

#endif
