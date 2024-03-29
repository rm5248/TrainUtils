/**
 * Get and parse Loconet information.
 * 
 * @Author Robert Middleton
 */
#ifndef LOCONET_BUFFER_H
#define LOCONET_BUFFER_H

#include <inttypes.h>

#ifdef	__cplusplus
extern "C" {
#endif

//
// Macro definitions
//
#define LN_CHECK_BIT(number,bit)   (number & (0x1 << bit))
#define LN_SET_BIT(number, bit)    (number |= (0x1 << bit))
#define LN_CLEAR_BIT(number, bit)  (number &= (~(0x1 << bit)))

//
// Useful macros for setting various bits
//
#define LOCONET_SET_LOCO_ADDR(ln, addr) ln.addr.locoAddrHi = ((addr & 0x7F) << 7); \
    ln.addr.locoAddrLo = (addr & 0x7F);
//NOTE NOTE NOTE: the following two macros are REVERSED from what the
//loconet spec says that they should be.  The spec says that bit 5 set
//is forward, however the DT400 sends bit 5 set = reversed.
#define LOCONET_SET_DIRECTION_REV(byte) LN_SET_BIT(byte,5)
#define LOCONET_SET_DIRECTION_FWD(byte) LN_CLEAR_BIT(byte,5)
#define LOCONET_GET_DIRECTION_REV(byte) LN_CHECK_BIT(byte,5)

//
// Struct Definitions
//

// Command = 0xBF
struct loconet_loco_address {
	uint8_t locoAddrHi;
	uint8_t locoAddrLo;
};

// Command = 0xBD
struct loconet_switch_ack {
	uint8_t switch1;
	uint8_t switch2;
};

// Command = 0xBC
struct loconet_switch_state {
	uint8_t	switch1;
	uint8_t switch2;
};

// Command = 0xBB
struct loconet_request_slot_data {
	uint8_t slot;
	uint8_t nul;
};

// Command = 0xBA
struct loconet_move_slot {
	uint8_t source;
	uint8_t slot;
};

// Command = 0xB9
struct loconet_link_slots {
	uint8_t slot1;
	uint8_t slot2;
};

// Command = 0xB8
struct loconet_unlink_slots {
	uint8_t slot1;
	uint8_t slot2;
};

// 0xBF - 0xB8 have responses

// Command = 0xB6
struct loconet_consist {
	uint8_t slot;
	uint8_t direction;
};

// Command = 0xB5
struct loconet_write_stat1 {
	uint8_t slot;
	uint8_t stat1;
};

// Command = 0xB4
struct loconet_ack {
	uint8_t lopc; // Long opcode, i.e. OPCODE & 0x7F
	uint8_t ack; //ack status
};

// Command = 0xB2
struct loconet_inputs {
	uint8_t in1;
	uint8_t in2;
};

// Command = 0xB1
struct loconet_turnout {
	uint8_t sn1;
	uint8_t sn2;
};

// Command = 0xB0
struct loconet_request_switch {
	uint8_t sw1;
	uint8_t sw2;
};

// Command = 0xA2
struct loconet_sound {
	uint8_t slot;
	uint8_t snd;
};

// Command = 0xA1
struct loconet_direction {
	uint8_t slot;
	uint8_t dir_funcs;
};

// Command = 0xA0
struct loconet_speed {
	uint8_t slot;
	uint8_t speed;
};

// Command = 0xEF
struct loconet_slot_data {
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
};

// Command = 0xEF, slot = 0x7B
struct loconet_clock_slot_data {
    uint8_t len;
    uint8_t slot;
    uint8_t clock_rate;
    uint8_t frac_minsl;
    uint8_t frac_minsh;
    uint8_t mins_60;
    uint8_t track;
    uint8_t hours_24;
    uint8_t days;
    uint8_t clock_ctl;
    uint8_t id1;
    uint8_t id2;
};

struct loconet_message {
	uint8_t opcode;
	union{
        struct loconet_loco_address 	addr;
        struct loconet_switch_ack 		switch_ack;
        struct loconet_switch_state 	switch_state;
        struct loconet_request_slot_data 	req_slot_data;
        struct loconet_move_slot 		move_slot;
        struct loconet_link_slots		link_slot;
        struct loconet_unlink_slots		unlink_slot;
        struct loconet_consist			consist;
        struct loconet_write_stat1		stat1;
        struct loconet_ack				ack;
        struct loconet_inputs			inputs;
        struct loconet_turnout			turnout;
        struct loconet_request_switch	req_switch;
        struct loconet_sound			sound;
        struct loconet_direction		direction_functions;
        struct loconet_speed			speed;
        struct loconet_slot_data		slot_data;
        struct loconet_clock_slot_data  clock_slot_data;
		uint8_t data[ 16 ];
	};
};
	
/**
 * Represents the current loconet time.  Use ln_get_time
 * to get the current loconet time.  The time will be valid
 * as soon as a read for slot 123 comes in.
 */
struct loconet_time {
    int hours;
    int minutes;
};

// What state the loconet currently is
enum loconet_state {
	LN_IDLE,
	LN_RX,
	LN_TX,
	LN_COLLISION,
	LN_CD_BACKOFF,
	LN_CD_BACKOFF_ADDITIONAL,
	LN_WAIT_MASTER
};

enum loconet_slot_status {
    LN_SLOT_STATUS_FREE,
    LN_SLOT_STATUS_COMMON,
    LN_SLOT_STATUS_IDLE,
    LN_SLOT_STATUS_IN_USE,
};

struct loconet_context;

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

#define LN_SLOT_STATUS(LnMessage) ((LnMessage.slot_data.stat & ( 0x03 << 4 )) >> 4)

//
// Library errors
//
#define LN_OK 0
#define LN_ERROR_INVALID_ARG -1
#define LN_ERROR_INVALID_CHECKSUM -2
#define LN_ERROR_INVALID_SWITCH_NUM -3
#define LN_ERROR_LOCO_ALREADY_SELECTED -4
#define LN_ERROR_INVALID_LOCO_NUMBER -5
#define LN_ERROR_NO_LOCO_SELECTED -6
#define LN_ERROR_INVALID_SENSOR_NUM -7

//
// Function typedefs
//

/**
 * This function is called to start a timer
 * The parameter is the number of microseconds to set the timer for
 * Note that this routine should modify whatever timer it creates,
 * instead of creating a new timer each time.  
 */
typedef void (*loconet_timer_start_function)( struct loconet_context* ctx, uint32_t );

/**
 * This function is called to actually write a byte out to the bus.
 * This function should wait until the byte has actually been written
 */
typedef void (*loconet_write_byte_function)( struct loconet_context* ctx, uint8_t );

/**
 * This function is called to write an entire block of data out to
 * loconet.  This is used when what you are interfacing with handles
 * the loconet collisions properly(e.g. PR3, PR4, LoconetTCP).
 */
typedef void (*loconet_write_interlocked_function)( struct loconet_context* ctx, uint8_t* data, int len );

/**
 * This callback will be called when a message comes in from the loconet bus
 */
typedef void(*loconet_incoming_message)( struct loconet_context* ctx, struct loconet_message* message);

//
// Function Definitions
//

/**
 * Initialize a new loconet_context for low-level communications(e.g. bytes/bits are written out one at a time)
 *
 * @param timerStartFn A function which will start a timer
 * @param writeFn The function to call to write a byte out to the bus
 * @param additionalDelay How many more uS to wait before attempting network access
 */
struct loconet_context* loconet_context_new( loconet_timer_start_function start, loconet_write_byte_function write );

/**
 * Initialize a new loconet_context where writes are interlocked(meaning, you can send the bytes and no
 * collision checking needs to happen).
 *
 * @param writeInterlock
 * @return
 */
struct loconet_context* loconet_context_new_interlocked( loconet_write_interlocked_function writeInterlock );

void loconet_context_free( struct loconet_context* context );

void loconet_context_set_additional_delay( struct loconet_context* ctx, uint8_t additionalDelay );

void loconet_context_set_message_callback(struct loconet_context* ctx, loconet_incoming_message callback_function );

/**
 * Set if we should ignore the state or not.
 *
 * If you are running on a system where you are directly connected to the Loconet bus,
 * don't set this.  Ignoring the state is fine if there is already a device which will
 * handle the backoff stuff(e.g. a PR3).
 *
 * This is automatically set when creating a loconet_context with loconet_context_new_interlocked
 *
 * @param ctx
 * @param ignore_state
 */
void loconet_context_set_ignore_state( struct loconet_context* ctx, int ignore_state );

/**
 * Process the bytes in our buffer.
 * This should be called after adding bytes with loconet_incoming_byte or loconet_incoming_bytes.
 */
int loconet_context_process( struct loconet_context* ctx );

/**
 * Write a message to the bus.  Will not return until the message
 * has been written OR it could not transmit the message
 *
 * @return 1 on success, 0 otherwise
 */
int loconet_context_write_message( struct loconet_context* ctx, struct loconet_message* );

/**
 * Get the state of loconet
 */
enum loconet_state loconet_context_get_state( struct loconet_context* ctx );

/**
 * Call this when the timer fires
 */
void loconet_context_timer_fired( struct loconet_context* ctx );

/**
 * Call this when a new byte comes in.
 * This routine may be called from an ISR.
 * If this routine is NOT called from an ISR,
 * you must properly mutex your calls to 
 * ln_incoming_byte and ln_read_message
 */
void loconet_context_incoming_byte( struct loconet_context* ctx, uint8_t byte );

/**
 * Call this with new bytes that have been received over Loconet.
 *
 * @param ctx
 * @param bytes
 * @param bytes_len
 * @return
 */
int loconet_context_incoming_bytes( struct loconet_context* ctx, void* bytes, int bytes_len );

/**
 * Get the current loconet time.
 *
 * @param ctx
 * @return
 */
struct loconet_time loconet_context_current_time( struct loconet_context* ctx );

void* loconet_context_user_data( struct loconet_context* ctx );

void loconet_context_set_user_data( struct loconet_context* ctx, void* user_data );

/**
 * Helper function to get how long the message is(in bytes).
 *
 * @param msg
 * @return
 */
int loconet_message_length( struct loconet_message* msg );

#ifdef	__cplusplus
}
#endif

#endif
