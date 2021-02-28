#include <string.h>
#include <inttypes.h>
#include <stdio.h>

#include "CabBus.h"
#include "Bitset.h"

#define COMMAND_STATION_START_BYTE  0xC0
#define REPEAT_SCREEN  0x7E
#define TOP_LEFT_LCD  0x00
#define TOP_RIGHT_LCD 0x01
#define BOTTOM_LEFT_LCD 0x02
#define BOTTOM_RIGHT_LCD  0x03
#define MOVE_LCD_CURSOR 0x08
#define PRINT_CHAR_BACKUP 0x09
#define PRINT_CHAR_FORWARD 0x0A
#define CLEAR_DISPLAY_HOME_CURSOR 0x0D
#define CURSOR_OFF 0x0E
#define CURSOR_ON 0x0F

//key definitions
#define NO_KEY  0x7D
#define ENTER	0x40
#define PROGRAM_KEY 0x41
#define RECALL_KEY 0x42
#define DIRECTION_KEY 0x43
#define CONSIST_KEY 0x44
#define ADD_LOCO_CONSIST_KEY 0x45
#define DEL_LOCO_CONSIST_KEY 0x46
#define KILL_CONSIST_KEY 0x47
#define SELECT_LOCO_KEY  0x48
#define HORN_KEY
#define STEP_FASTER_KEY  0x4A
#define STEP_SLOWER_KEY  0x4B
#define ESTOP_KEY 0x4C
#define BELL_KEY 0x4D
#define SEL_ACCY_KEY 0x4E
#define KEY_0	0x50
#define KEY_1	0x51
#define KEY_2	0x52
#define KEY_3	0x53
#define KEY_4	0x54
#define KEY_5	0x55
#define KEY_6	0x56
#define KEY_7	0x57
#define KEY_8	0x58
#define KEY_9	0x59
#define SPEED_INC_FAST_KEY 0x5A
#define SPEED_DEC_FAST_KEY 0x5B
#define SELECT_MACRO_KEY 0x5C
#define SPEED_STEP_CHANGE_KEY 0x5D
#define BRAKE_KEY 0x5E

#define CAB_GET_ASK_QUESTION(cab) CHECK_BIT(cab->dirty_screens, 5)
#define CAB_SET_ASK_QUESTION(cab) SET_BIT(cab->dirty_screens, 5)
#define CAB_CLEAR_ASK_QUESTION(cab) CLEAR_BIT(cab->dirty_screens, 5)
#define CAB_GET_SELECTING_LOCO(cab) CHECK_BIT(cab->dirty_screens, 6)
#define CAB_SET_SELECTING_LOCO(cab) SET_BIT(cab->dirty_screens, 6)
#define CAB_CLEAR_SELECTING_LOCO(cab) CLEAR_BIT(cab->dirty_screens, 6)
//Macros for getting/setting dirty state of screens
#define CAB_SET_TOPLEFT_DIRTY(cab) SET_BIT(cab->dirty_screens, 3)
#define CAB_SET_BOTTOMLEFT_DIRTY(cab) SET_BIT(cab->dirty_screens, 2)
#define CAB_SET_TOPRIGHT_DIRTY(cab) SET_BIT(cab->dirty_screens, 1)
#define CAB_SET_BOTTOMRIGHT_DIRTY(cab) SET_BIT(cab->dirty_screens, 0)
#define CAB_GET_TOPLEFT_DIRTY(cab) CHECK_BIT(cab->dirty_screens, 3)
#define CAB_GET_BOTTOMLEFT_DIRTY(cab) CHECK_BIT(cab->dirty_screens, 2)
#define CAB_GET_TOPRIGHT_DIRTY(cab) CHECK_BIT(cab->dirty_screens, 1)
#define CAB_GET_BOTTOMRIGHT_DIRTY(cab) CHECK_BIT(cab->dirty_screens, 0)
//Clearing dirty bits
#define CAB_SET_TOPLEFT_CLEAN(cab) CLEAR_BIT(cab->dirty_screens, 3)
#define CAB_SET_BOTTOMLEFT_CLEAN(cab) CLEAR_BIT(cab->dirty_screens, 2)
#define CAB_SET_TOPRIGHT_CLEAN(cab) CLEAR_BIT(cab->dirty_screens, 1)
#define CAB_SET_BOTTOMRIGHT_CLEAN(cab) CLEAR_BIT(cab->dirty_screens, 0)
//check if we have dirty screens
#define CAB_HAS_DIRTY_SCREENS(cab) (cab->dirty_screens & 0x0F)

static const char* FWD = "FWD";
static const char* REV = "REV";

//
// Local Structs
//

struct Cab {
    //the number of this cab, 0-64
    uint8_t number;
    //the current speed of this cab(0-127 speed steps).  Top bit = direction
    uint8_t speed;
    //the current locomotive number that we are controlling
    uint16_t loco_number;
    //bitfields representing the functions that we are using(displayed on the cab)
    uint8_t functions;
    //lower 4 bits correspond to the dirtyness of the screens
    //upper 4 bits = flags
    // 0x01 << 5 = asking a question
    uint8_t dirty_screens;
    char topRow[16];
    char bottomRow[16];
    //the latest command from the cab
    struct cab_command command;
    uint32_t last_ping;
    // Set this field if we need to put the cursor at a
    // specific location
    int8_t new_cursor_position;
    uint16_t current_selecting_loco;
    //any other data that you want to associate with this cab.
    void* user_data;
};

//
// Local variables
//

//Contains the data for all the cabs
static struct Cab allCabs[ 64 ];
static uint8_t currentCabAddr;
static uint8_t outputBuffer[ 10 ];

// functions to assist us in communications
static cab_delay_fn delayFunction;
static cab_write_fn writeFunction;
static cab_incoming_data incomingFunction;

// storage for incoming bytes
volatile uint8_t firstByte;
volatile uint8_t secondByte;
volatile uint8_t byteStatus;

// The current ping#, if we did not ping this cab the last time
// around set the screens to be dirty
static uint32_t pingNum;

//
// Local functions
//

/**
 * Move the cursor to the location specified on the LCD screen.
 * @param location The location, 0-32
 */
static void cabbus_move_cursor_to_location( uint8_t location ){
    outputBuffer[ 0 ] = COMMAND_STATION_START_BYTE | MOVE_LCD_CURSOR;
    if( location < 16 ) {
        outputBuffer[ 1 ] = 0x80 + location;
    }else if( location >= 16 ){
        outputBuffer[ 1 ] = 0xC0 + location - 16;
    }
    writeFunction( outputBuffer, 2 );
}

/**
 * Print the specified ASCII char, backup to the same location
 * @param character
 */
static void cabbus_print_char_backup( char character ){
    outputBuffer[ 0 ] = COMMAND_STATION_START_BYTE | PRINT_CHAR_BACKUP;
    outputBuffer[ 1 ] = character;
    writeFunction( outputBuffer, 2 );
}

/**
 * Print the specified ASCII char, move the cursor forward
 * @param character
 */
static void cabbus_print_char( char character ){
    outputBuffer[ 0 ] = COMMAND_STATION_START_BYTE | PRINT_CHAR_FORWARD;
    outputBuffer[ 1 ] = character;
    writeFunction( outputBuffer, 2 );
}

/**
 * Clear the screen and home the cursor
 */
static void cabbus_clear_screen_home_cursor(){
    outputBuffer[ 0 ] = COMMAND_STATION_START_BYTE | CLEAR_DISPLAY_HOME_CURSOR;
    writeFunction( outputBuffer, 1 );
}

static void cabbus_cursor_on(){
    outputBuffer[ 0 ] = COMMAND_STATION_START_BYTE | CURSOR_ON;
    writeFunction( outputBuffer, 1 );
}

static void cabbus_cursor_off(){
    outputBuffer[ 0 ] = COMMAND_STATION_START_BYTE | CURSOR_OFF;
    writeFunction( outputBuffer, 1 );
}

static char simp_atoi(int number) {
    switch (number) {
        case 9:
            return '9';
        case 8:
            return '8';
        case 7:
            return '7';
        case 6:
            return '6';
        case 5:
            return '5';
        case 4:
            return '4';
        case 3:
            return '3';
        case 2:
            return '2';
        case 1:
            return '1';
        case 0:
            return '0';
    }

    return '-';
}

static void cab_reset(struct Cab* cab) {
    int x;

    char tempBuffer[ 9 ];
    snprintf(tempBuffer, 9, "%s:%3d", cab->speed & 0x80 ? FWD : REV, cab->speed & 0x7F);
    memcpy(cab->bottomRow, tempBuffer, 8);
    cab->bottomRow[ 7 ] = ' ';

    for (x = 0; x < 8; x++) {
        if (cab->functions & (0x01 << x)) {
            cab->bottomRow[ x + 8 ] = simp_atoi(x);
            if (x == 0) {
                cab->bottomRow[ x + 8 ] = 'L';
            }
        } else {
            cab->bottomRow[ x + 8 ] = '-';
        }
    }

    CAB_SET_BOTTOMLEFT_DIRTY(cab);
    CAB_SET_BOTTOMRIGHT_DIRTY(cab);
}

//
// Cabbus functions
//

void cabbus_init(cab_delay_fn inDelay, cab_write_fn inWrite, cab_incoming_data inData) {
    unsigned char x;

    delayFunction = inDelay;
    writeFunction = inWrite;
    incomingFunction = inData;

    memset( allCabs, 0, sizeof( allCabs ) );

    for (x = 0; x < 64; x++) {
        allCabs[ x ].number = x;
        allCabs[ x ].new_cursor_position = -1;
        // Set defaults for our cabs; will also set the screens to be dirty
        cabbus_set_loco_number(&allCabs[ x ], 44);
        cabbus_set_loco_speed(&allCabs[ x ], 0);
        cabbus_set_time(&allCabs[ x ], 5, 55, 1);
        cabbus_set_functions(&allCabs[ x ], 1, 1);
        cabbus_set_direction(&allCabs[ x ], FORWARD);
        allCabs[ x ].command.command = CAB_CMD_NONE;
        allCabs[ x ].dirty_screens = 0x0F; // All screens dirty
    }

    currentCabAddr = 0;
    pingNum = 0;
}

static int cabbus_handle_select_loco(struct Cab* current, int keyByte){
    if (keyByte == SELECT_LOCO_KEY) {
        //send the message 'enter loco:' to the cab
        //loco number prints on LCD screen starting at 0xCC
        current->current_selecting_loco = 0;
        current->command.sel_loco.flags = 1;

        memcpy(current->bottomRow, "ENTER LOCO:     ", 16);
        CAB_SET_BOTTOMLEFT_DIRTY(current);
        CAB_SET_BOTTOMRIGHT_DIRTY(current);
        CAB_SET_SELECTING_LOCO(current);
        current->new_cursor_position = 16 + 12; // 12 bytes into 2nd row
        return 1;
    }

    // At this point, we must be in our selecting loco state for
    // anything useful to happen
    if( !CAB_GET_SELECTING_LOCO(current) ){
        return 0;
    }

    if( current->new_cursor_position >= 0 ){
        return 0;
    }

    if( keyByte == KEY_0 ){
        cabbus_print_char( '0' );
        if( current->current_selecting_loco == 0 ){
            // Select short address
            current->command.sel_loco.flags = 0;
        }else{
            current->current_selecting_loco *= 10;
        }
    }else if( keyByte == KEY_1 ){
        cabbus_print_char( '1' );
        current->current_selecting_loco *= 10;
	current->current_selecting_loco += 1;
    }else if( keyByte == KEY_2 ){
        cabbus_print_char( '2' );
        current->current_selecting_loco *= 10;
	current->current_selecting_loco += 2;
    }else if( keyByte == KEY_3 ){
        cabbus_print_char( '3' );
        current->current_selecting_loco *= 10;
	current->current_selecting_loco += 3;
    }else if( keyByte == KEY_4 ){
        cabbus_print_char( '4' );
        current->current_selecting_loco *= 10;
	current->current_selecting_loco += 4;
    }else if( keyByte == KEY_5 ){
        cabbus_print_char( '5' );
        current->current_selecting_loco *= 10;
	current->current_selecting_loco += 5;
    }else if( keyByte == KEY_6 ){
        cabbus_print_char( '6' );
        current->current_selecting_loco *= 10;
	current->current_selecting_loco += 6;
    }else if( keyByte == KEY_7 ){
        cabbus_print_char( '7' );
        current->current_selecting_loco *= 10;
	current->current_selecting_loco += 7;
    }else if( keyByte == KEY_8 ){
        cabbus_print_char( '8' );
        current->current_selecting_loco *= 10;
	current->current_selecting_loco += 8;
    }else if( keyByte == KEY_9 ){
        cabbus_print_char( '9' );
        current->current_selecting_loco *= 10;
	current->current_selecting_loco += 9;
    }else if( keyByte == ENTER ){
        current->command.command = CAB_CMD_SEL_LOCO;
        current->command.sel_loco.address = current->current_selecting_loco;
        CAB_CLEAR_SELECTING_LOCO(current);
        cab_reset(current);
    }

    return 1;
}

/**
 * Process the button press from the specified cab.
 *
 * @param current
 * @param keyByte
 * @return TRUE if we need to re-ping this cab
 */
static int cabbus_process_button_press(struct Cab* current, int keyByte) {
    if( cabbus_handle_select_loco( current, keyByte ) ){
        return 0;
    }

    if (keyByte == REPEAT_SCREEN) {
        // set all screens to be dirty
        allCabs[ currentCabAddr ].dirty_screens = 0x0F;
    } else if (keyByte == SELECT_LOCO_KEY) {
        //send the message 'enter loco:' to the cab
        //loco number prints on LCD screen starting at 0xCC

        memcpy(current->bottomRow, "ENTER LOCO:     ", 16);
        CAB_SET_BOTTOMLEFT_DIRTY(current);
        CAB_SET_BOTTOMRIGHT_DIRTY(current);
        CAB_SET_SELECTING_LOCO(current);
        return 1;
    } else if (keyByte == ENTER) {
        //reset all screens
        cab_reset(current);
        if (CAB_GET_SELECTING_LOCO(current)) {
            current->command.command = CAB_CMD_SEL_LOCO;
            CAB_CLEAR_SELECTING_LOCO(current);
        }
    } else if (keyByte == KEY_0) {
        if (CAB_GET_ASK_QUESTION(current)) {
            cab_reset(current);
            current->command.command = CAB_CMD_RESPONSE;
            current->command.response.response = 0;
        }else if(CAB_GET_SELECTING_LOCO(current)){
            current->bottomRow[ 12 ] = '0';
            CAB_SET_BOTTOMRIGHT_DIRTY(current);
	}

    } else if (keyByte == KEY_1) {
        if (CAB_GET_ASK_QUESTION(current)) {
            cab_reset(current);
            current->command.command = CAB_CMD_RESPONSE;
            current->command.response.response = 1;
        }
    } else if (keyByte == STEP_FASTER_KEY) {
        if ((current->speed & 0x7F) != 127) {
            current->command.command = CAB_CMD_SPEED;
            //current->speed = current->speed + 1;
            current->command.speed.speed = (current->speed & 0x7F) + 1;
        }
    } else if (keyByte == STEP_SLOWER_KEY) {
        if ((current->speed & 0x7F) != 0) {
            current->command.command = CAB_CMD_SPEED;
            //current->speed = current->speed - 1;
            current->command.speed.speed = (current->speed & 0x7F) - 1;
        }
    } else if (keyByte == DIRECTION_KEY) {
        current->command.command = CAB_CMD_DIRECTION;
        if (current->speed & 0x80) {
            //we are going forward, set to backwards
            current->command.direction.direction = REVERSE;
        } else {
            current->command.direction.direction = FORWARD;
        }
    } else if (keyByte == ESTOP_KEY) {
        current->command.command = CAB_CMD_ESTOP;
    }

    return 0;
}

/**
 * Output the current screens to the cab specified
 * @param current
 */
static void cabbus_output_screens(struct Cab* current) {
    if (CAB_HAS_DIRTY_SCREENS(current)) {
        outputBuffer[ 0 ] = COMMAND_STATION_START_BYTE;
        // send out the first dirty screen!
        if (CAB_GET_TOPLEFT_DIRTY(current)) {
            memcpy(&(outputBuffer[1]), current->topRow, 8);
            outputBuffer[ 0 ] |= TOP_LEFT_LCD;
            CAB_SET_TOPLEFT_CLEAN(current);
        } else if (CAB_GET_BOTTOMLEFT_DIRTY(current)) {
            memcpy(&(outputBuffer[1]), current->bottomRow, 8);
            outputBuffer[ 0 ] |= BOTTOM_LEFT_LCD;
            CAB_SET_BOTTOMLEFT_CLEAN(current);
        } else if (CAB_GET_TOPRIGHT_DIRTY(current)) {
            memcpy(&(outputBuffer[1]), current->topRow + 8, 8);
            outputBuffer[ 0 ] |= TOP_RIGHT_LCD;
            CAB_SET_TOPRIGHT_CLEAN(current);
        } else if (CAB_GET_BOTTOMRIGHT_DIRTY(current)) {
            memcpy(&(outputBuffer[1]), current->bottomRow + 8, 8);
            outputBuffer[ 0 ] |= BOTTOM_RIGHT_LCD;
            CAB_SET_BOTTOMRIGHT_CLEAN(current);
        }

        //Delay, the Power Cab is pretty slow it seems
        //delayFunction(2);
        writeFunction(outputBuffer, 9);
        return;
    }

    if( current->new_cursor_position >= 0 ){
        cabbus_move_cursor_to_location( current->new_cursor_position );
        current->new_cursor_position = -2;
        return;
    }

    if( current->new_cursor_position == -2 ){
        cabbus_cursor_on();
        current->new_cursor_position = -1;
    }
}


struct Cab* cabbus_ping_next() {
    static struct Cab* current;
    //set this to true if we need to re-ping the current address
    int rePing = 0;

    currentCabAddr++;
    if (currentCabAddr == 1) currentCabAddr++; //don't ping address 1
    if (currentCabAddr == 2) currentCabAddr++; //don't ping address 2 either

    if (currentCabAddr == 64) {
        currentCabAddr = 0; //only up to 63 cab
        pingNum++;
    }

    do {
        //Go and ping the next address
        outputBuffer[ 0 ] = 0x80 | currentCabAddr;

        writeFunction(outputBuffer, 1);

        //Delay to make sure that we get a response back
        delayFunction(3);

        if (pingNum - allCabs[ currentCabAddr ].last_ping > 1) {
            //we haven't seen this guy, set all screens to dirty
            allCabs[ currentCabAddr ].dirty_screens |= 0x0F;
        }

        if (byteStatus & 0x01) {
            //we have a response back from a cab
            unsigned char knobByte;
            unsigned char keyByte;
            unsigned int loopTimes = 0;

            current = &allCabs[ currentCabAddr ];
            current->command.command = CAB_CMD_NONE;

            loopTimes = 0;
            while (!(byteStatus & 0x02)) {
                ++loopTimes;
                if (loopTimes > 1000) {
                    byteStatus = 0x00;
                    return NULL;
                }
            }
            knobByte = secondByte;
            keyByte = firstByte;
            byteStatus = 0x00;

            current->last_ping = pingNum;

            if (keyByte != NO_KEY) {
                rePing = cabbus_process_button_press(current, keyByte);
printf( "%d\n", rePing );
if( rePing ) printf( "re-ping\n" );
            }

            cabbus_output_screens(current);

            //we got a response, tell the master that this cab exists,
            //but only if we don't have to re-ping
            if( !rePing ) return current;
        }
    } while (rePing);

    return NULL; //unable to ping this cab
}

void cabbus_ping_step1(){
    currentCabAddr++;
    if (currentCabAddr == 1) currentCabAddr++; //don't ping address 1
    if (currentCabAddr == 2) currentCabAddr++; //don't ping address 2 either

    if (currentCabAddr == 64) {
        currentCabAddr = 0; //only up to 63 cab
        pingNum++;
    }

    //Go and ping the next address
    outputBuffer[ 0 ] = 0x80 | currentCabAddr;

    writeFunction(outputBuffer, 1);

    //Delay to make sure that we get a response back
    delayFunction(1);
}

struct Cab* cabbus_ping_step2(){
	struct Cab* current = NULL;
	int rePing = 0;

    if (pingNum - allCabs[ currentCabAddr ].last_ping > 1) {
        //we haven't seen this guy, set all screens to dirty
        allCabs[ currentCabAddr ].dirty_screens |= 0x0F;
    }

    if (byteStatus & 0x01) {
        //we have a response back from a cab
        unsigned char knobByte;
        unsigned char keyByte;
        unsigned int loopTimes = 0;

        current = &allCabs[ currentCabAddr ];
        current->command.command = CAB_CMD_NONE;

        loopTimes = 0;
        while (!(byteStatus & 0x02)) {
            ++loopTimes;
            if (loopTimes > 1000) {
                byteStatus = 0x00;
                return NULL;
            }
        }
        knobByte = secondByte;
        keyByte = firstByte;
        byteStatus = 0x00;

        current->last_ping = pingNum;

        if (keyByte != NO_KEY) {
            rePing = cabbus_process_button_press(current, keyByte);
			if( rePing ){
    			//currentCabAddr--;
			}
        }

        cabbus_output_screens(current);
    }

    return current;
}

void cabbus_set_loco_number(struct Cab* cab, int number) {
    //first off, quick sanity check here
    if (number > 9999) {
        number = 9999;
    }

    if (cab->loco_number != number) {
        cab->loco_number = number;
        char tempBuffer[ 9 ];
        snprintf(tempBuffer, 9, "LOC:%3d", number);
        memcpy(cab->topRow, tempBuffer, 8);
        cab->topRow[ 7 ] = ' ';
        CAB_SET_TOPLEFT_DIRTY(cab);
    }
}

void cabbus_set_loco_speed(struct Cab* cab, uint8_t speed) {
    uint8_t userSpeed = speed & 0x7F;

    if (cab == NULL) return;

    cab->speed = (cab->speed & 0x80) | userSpeed;
    //need a temp buffer, sprintf will put a NULL at the end, we
    //only want 8 bytes
    char tempBuffer[ 9 ];
    snprintf(tempBuffer, 9, "%s:%3d", cab->speed & 0x80 ? FWD : REV, userSpeed);
    memcpy(cab->bottomRow, tempBuffer, 8);
    cab->bottomRow[ 7 ] = ' ';
    CAB_SET_BOTTOMLEFT_DIRTY(cab);
}

void cabbus_set_time(struct Cab* cab, char hour, char minute, char am) {
    const char* AM = "AM";
    const char* PM = "PM";

    if (cab == NULL) return;

    char tempBuffer[ 9 ];
    snprintf(tempBuffer, 9, "%2d:%02d %s", hour, minute, am ? AM : PM);
    memcpy(cab->topRow + 8, tempBuffer, 8);
    CAB_SET_TOPRIGHT_DIRTY(cab);
}

void cabbus_set_functions(struct Cab* cab, char functionNum, char on) {
    unsigned char x;

    if (cab == NULL) return;

    if (on) {
        cab->functions |= (0x01 << functionNum);
    } else {
        cab->functions &= ~(0x01 << functionNum);
    }

    for (x = 0; x < 8; x++) {
        if (cab->functions & (0x01 << x)) {
            cab->bottomRow[ x + 8 ] = simp_atoi(x);
            if (x == 0) {
                cab->bottomRow[ x + 8 ] = 'L';
            }
        } else {
            cab->bottomRow[ x + 8 ] = '-';
        }
    }

    CAB_SET_BOTTOMLEFT_DIRTY(cab);
}

void cabbus_set_direction(struct Cab* cab, enum Direction direction) {
    if (cab == NULL) return;
    if (direction == FORWARD) {
        cab->speed |= 0x80;
    } else {
        cab->speed &= ~(0x80);
    }

    //force an update
    cabbus_set_loco_speed(cab, cab->speed);
}

void cabbus_incoming_byte(uint8_t byte) {
    if (byteStatus == 0x00) {
        //no bytes, put in first byte
        firstByte = byte;
        byteStatus = 0x01;
    } else if (byteStatus == 0x01) {
        secondByte = byte;
        byteStatus = 0x03;
    } else {
        byteStatus = 0x00;
    }
}

uint16_t cabbus_get_loco_number(struct Cab* cab) {
    if (cab == NULL) return 0;
    return cab->loco_number;
}

struct cab_command* cabbus_get_command(struct Cab* cab) {
    if (cab == NULL) return NULL;
    return &(cab->command);
}

void cabbus_ask_question(struct Cab* cab, const char* message) {
    if (cab == NULL) return;
    if (strlen(message) > 16) {
        return;
    }

    CAB_SET_ASK_QUESTION(cab);

    char tempBuffer[ 17 ];
    memset(tempBuffer, ' ', 16);
    snprintf(tempBuffer, 17, "%s", message);
    memcpy(cab->bottomRow, tempBuffer, 16);
    CAB_SET_BOTTOMLEFT_DIRTY(cab);
    CAB_SET_BOTTOMRIGHT_DIRTY(cab);
}

uint8_t cabbus_get_cab_number(struct Cab* cab) {
    if (cab == NULL) return 0;
    return cab->number;
}

void cabbus_user_message(struct Cab* cab, const char* message) {
    if (cab == NULL) return;
    if (strlen(message) > 16) {
        return;
    }

    char tempBuffer[ 17 ];
    memset(tempBuffer, ' ', 16);
    snprintf(tempBuffer, 17, "%s", message);
    memcpy(cab->bottomRow, tempBuffer, 16);
    CAB_SET_BOTTOMLEFT_DIRTY(cab);
    CAB_SET_BOTTOMRIGHT_DIRTY(cab);
}

void cabbus_set_user_data(struct Cab* cab, void* data) {
    if (cab == NULL) return;
    cab->user_data = data;
}

void* cabbus_get_user_data(struct Cab* cab) {
    if (cab == NULL) return NULL;
    return cab->user_data;
}

int cabbus_get_function(struct Cab* cab, uint8_t function) {
    if (cab == NULL) return 0;
    if (cab->functions & (0x01 << function)) {
        return 1;
    }

    return 0;
}
