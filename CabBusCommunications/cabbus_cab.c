#include "cabbus_cab.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define CABBUS_SECRET
#include "cabbus_private.h"
#include "Bitset.h"

#define CURSOR_POSITION_NONE -1
#define CURSOR_POSITION_ENABLE -2
#define CURSOR_POSITION_DISABLE -3

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
#define CURRENT_CONTROL_LOCOMOTIVE_INFO 0x5B

#define CAB_GET_ASK_QUESTION(cab) BIT_CHECK(cab->dirty_screens, 5)
#define CAB_SET_ASK_QUESTION(cab) BIT_SET(cab->dirty_screens, 5)
#define CAB_CLEAR_ASK_QUESTION(cab) BIT_CLEAR(cab->dirty_screens, 5)
#define CAB_GET_SELECTING_LOCO(cab) BIT_CHECK(cab->dirty_screens, 6)
#define CAB_SET_SELECTING_LOCO(cab) BIT_SET(cab->dirty_screens, 6)
#define CAB_CLEAR_SELECTING_LOCO(cab) BIT_CLEAR(cab->dirty_screens, 6)
//Macros for getting/setting dirty state of screens
#define CAB_SET_TOPLEFT_DIRTY(cab) BIT_SET(cab->dirty_screens, 3)
#define CAB_SET_BOTTOMLEFT_DIRTY(cab) BIT_SET(cab->dirty_screens, 2)
#define CAB_SET_TOPRIGHT_DIRTY(cab) BIT_SET(cab->dirty_screens, 1)
#define CAB_SET_BOTTOMRIGHT_DIRTY(cab) BIT_SET(cab->dirty_screens, 0)
#define CAB_GET_TOPLEFT_DIRTY(cab) BIT_CHECK(cab->dirty_screens, 3)
#define CAB_GET_BOTTOMLEFT_DIRTY(cab) BIT_CHECK(cab->dirty_screens, 2)
#define CAB_GET_TOPRIGHT_DIRTY(cab) BIT_CHECK(cab->dirty_screens, 1)
#define CAB_GET_BOTTOMRIGHT_DIRTY(cab) BIT_CHECK(cab->dirty_screens, 0)
//Clearing dirty bits
#define CAB_SET_TOPLEFT_CLEAN(cab) BIT_CLEAR(cab->dirty_screens, 3)
#define CAB_SET_BOTTOMLEFT_CLEAN(cab) BIT_CLEAR(cab->dirty_screens, 2)
#define CAB_SET_TOPRIGHT_CLEAN(cab) BIT_CLEAR(cab->dirty_screens, 1)
#define CAB_SET_BOTTOMRIGHT_CLEAN(cab) BIT_CLEAR(cab->dirty_screens, 0)
//check if we have dirty screens
#define CAB_HAS_DIRTY_SCREENS(cab) (cab->dirty_screens & 0x0F)

static const char* FWD = "FWD";
static const char* REV = "REV";

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


/**
 * Move the cursor to the location specified on the LCD screen.
 * @param location The location, 0-32
 */
static void cabbus_cab_move_cursor_to_location( cab_write_fn write_fn, uint8_t location ){
    uint8_t output[2];
    output[ 0 ] = COMMAND_STATION_START_BYTE | MOVE_LCD_CURSOR;
    if( location < 16 ) {
        output[ 1 ] = 0x80 + location;
    }else if( location >= 16 ){
        output[ 1 ] = 0xC0 + location - 16;
    }
    write_fn( output, 2 );
}

/**
 * Print the specified ASCII char, backup to the same location
 * @param character
 */
static void cabbus_cab_print_char_backup( cab_write_fn write_fn, char character ){
    uint8_t output[2];
    output[ 0 ] = COMMAND_STATION_START_BYTE | PRINT_CHAR_BACKUP;
    output[ 1 ] = character;
    write_fn( output, 2 );
}

/**
 * Print the specified ASCII char, move the cursor forward
 * @param character
 */
static void cabbus_cab_print_char( cab_write_fn write_fn, char character ){
    uint8_t output[2];
    output[ 0 ] = COMMAND_STATION_START_BYTE | PRINT_CHAR_FORWARD;
    output[ 1 ] = character;
    write_fn( output, 2 );
}

/**
 * Clear the screen and home the cursor
 */
static void cabbus_cab_clear_screen_home_cursor( cab_write_fn write_fn ){
    uint8_t byte = COMMAND_STATION_START_BYTE | CLEAR_DISPLAY_HOME_CURSOR;
    write_fn( &byte, 1 );
}

static void cabbus_cab_cursor_on( cab_write_fn write_fn ){
    uint8_t byte = COMMAND_STATION_START_BYTE | CURSOR_ON;
    write_fn( &byte, 1 );
}

static void cabbus_cab_cursor_off( cab_write_fn write_fn ){
    uint8_t byte = COMMAND_STATION_START_BYTE | CURSOR_OFF;
    write_fn( &byte, 1 );
}

/**
 * Output the current screens to the cab specified
 * @param current
 */
void cabbus_cab_output_dirty_screens(cab_write_fn write_fn, struct cabbus_cab* current) {
    uint8_t output_buffer[9];

    if (CAB_HAS_DIRTY_SCREENS(current)) {
        output_buffer[ 0 ] = COMMAND_STATION_START_BYTE;
        // send out the first dirty screen!
        if (CAB_GET_TOPLEFT_DIRTY(current)) {
            memcpy(&(output_buffer[1]), current->topRow, 8);
            output_buffer[ 0 ] |= TOP_LEFT_LCD;
            CAB_SET_TOPLEFT_CLEAN(current);
        } else if (CAB_GET_BOTTOMLEFT_DIRTY(current)) {
            memcpy(&(output_buffer[1]), current->bottomRow, 8);
            output_buffer[ 0 ] |= BOTTOM_LEFT_LCD;
            CAB_SET_BOTTOMLEFT_CLEAN(current);
        } else if (CAB_GET_TOPRIGHT_DIRTY(current)) {
            memcpy(&(output_buffer[1]), current->topRow + 8, 8);
            output_buffer[ 0 ] |= TOP_RIGHT_LCD;
            CAB_SET_TOPRIGHT_CLEAN(current);
        } else if (CAB_GET_BOTTOMRIGHT_DIRTY(current)) {
            memcpy(&(output_buffer[1]), current->bottomRow + 8, 8);
            output_buffer[ 0 ] |= BOTTOM_RIGHT_LCD;
            CAB_SET_BOTTOMRIGHT_CLEAN(current);
        }

        //Delay, the Power Cab is pretty slow it seems
        //delayFunction(2);
        write_fn(output_buffer, 9);
        return;
    }

    if( current->new_cursor_position >= 0 ){
        cabbus_cab_move_cursor_to_location( write_fn, current->new_cursor_position );
        current->new_cursor_position = -2;
        return;
    }

    if( current->new_cursor_position == CURSOR_POSITION_ENABLE ){
        cabbus_cab_cursor_on( write_fn );
        current->new_cursor_position = CURSOR_POSITION_NONE;
    }

    if( current->new_cursor_position == CURSOR_POSITION_DISABLE ){
        cabbus_cab_cursor_off( write_fn );
        current->new_cursor_position = CURSOR_POSITION_NONE;
    }
}

void cabbus_cab_set_loco_number(struct cabbus_cab* cab, int number) {
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

void cabbus_cab_set_loco_speed(struct cabbus_cab* cab, uint8_t speed) {
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

void cabbus_cab_set_time(struct cabbus_cab* cab, char hour, char minute, char am) {
    const char* AM = "AM";
    const char* PM = "PM";

    if (cab == NULL) return;

    char tempBuffer[ 9 ];
    snprintf(tempBuffer, 9, "%2d:%02d %s", hour, minute, am ? AM : PM);
    memcpy(cab->topRow + 8, tempBuffer, 8);
    CAB_SET_TOPRIGHT_DIRTY(cab);
}

void cabbus_cab_set_functions(struct cabbus_cab* cab, char functionNum, char on) {
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

    CAB_SET_BOTTOMRIGHT_DIRTY(cab);
}

void cabbus_cab_set_direction(struct cabbus_cab* cab, enum cabbus_direction direction) {
    if (cab == NULL) return;
    if (direction == CAB_DIR_FORWARD) {
        cab->speed |= 0x80;
    } else {
        cab->speed &= ~(0x80);
    }

    //force an update
    cabbus_cab_set_loco_speed(cab, cab->speed);
}

void cabbus_cab_reset(struct cabbus_cab* cab) {
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


static int cabbus_cab_handle_select_loco(cab_write_fn write_fn, struct cabbus_cab* current, int keyByte){
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
        cabbus_cab_print_char( write_fn, '0' );
        if( current->current_selecting_loco == 0 ){
            // Select short address
            current->command.sel_loco.flags = 0;
        }else{
            current->current_selecting_loco *= 10;
        }
    }else if( keyByte == KEY_1 ){
        cabbus_cab_print_char( write_fn, '1' );
        current->current_selecting_loco *= 10;
    current->current_selecting_loco += 1;
    }else if( keyByte == KEY_2 ){
        cabbus_cab_print_char( write_fn, '2' );
        current->current_selecting_loco *= 10;
    current->current_selecting_loco += 2;
    }else if( keyByte == KEY_3 ){
        cabbus_cab_print_char( write_fn, '3' );
        current->current_selecting_loco *= 10;
    current->current_selecting_loco += 3;
    }else if( keyByte == KEY_4 ){
        cabbus_cab_print_char( write_fn, '4' );
        current->current_selecting_loco *= 10;
    current->current_selecting_loco += 4;
    }else if( keyByte == KEY_5 ){
        cabbus_cab_print_char( write_fn, '5' );
        current->current_selecting_loco *= 10;
    current->current_selecting_loco += 5;
    }else if( keyByte == KEY_6 ){
        cabbus_cab_print_char( write_fn, '6' );
        current->current_selecting_loco *= 10;
    current->current_selecting_loco += 6;
    }else if( keyByte == KEY_7 ){
        cabbus_cab_print_char( write_fn, '7' );
        current->current_selecting_loco *= 10;
    current->current_selecting_loco += 7;
    }else if( keyByte == KEY_8 ){
        cabbus_cab_print_char( write_fn, '8' );
        current->current_selecting_loco *= 10;
    current->current_selecting_loco += 8;
    }else if( keyByte == KEY_9 ){
        cabbus_cab_print_char( write_fn, '9' );
        current->current_selecting_loco *= 10;
    current->current_selecting_loco += 9;
    }else if( keyByte == ENTER ){
        if( current->current_selecting_loco != 0 ){
            current->command.command = CAB_CMD_SEL_LOCO;
            current->command.sel_loco.address = current->current_selecting_loco;
        }else{
            current->command.command = CAB_CMD_UNSELECT_LOCO;
        }
        CAB_CLEAR_SELECTING_LOCO(current);
        cabbus_cab_reset(current);
        current->new_cursor_position = CURSOR_POSITION_DISABLE;
    }

    return 1;
}

/**
 * Process the button press from the specified cab.
 *
 * @param current
 * @param keyByte
 */
void cabbus_cab_process_button_press(cab_write_fn write, struct cabbus_cab* current, int keyByte) {
    if( cabbus_cab_handle_select_loco( write, current, keyByte ) ){
        return;
    }

    if (keyByte == REPEAT_SCREEN) {
        // set all screens to be dirty
        current->dirty_screens = 0x0F;
    } else if (keyByte == ENTER) {
        //reset all screens
        cabbus_cab_reset(current);
    } else if (keyByte == KEY_0) {
        if (CAB_GET_ASK_QUESTION(current)) {
            CAB_CLEAR_ASK_QUESTION(current);
            cabbus_cab_reset(current);
            current->command.command = CAB_CMD_RESPONSE;
            current->command.response.response = 0;
        }else{
            current->command.command = CAB_CMD_FUNCTION;
            current->command.function.onoff = !cabbus_cab_get_function( current, 0 );
            current->command.function.function_number = 0;
        }
    } else if (keyByte == KEY_1) {
        if (CAB_GET_ASK_QUESTION(current)) {
            CAB_CLEAR_ASK_QUESTION(current);
            cabbus_cab_reset(current);
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
            current->command.direction.direction = CAB_DIR_REVERSE;
        } else {
            current->command.direction.direction = CAB_DIR_FORWARD;
        }
    } else if (keyByte == ESTOP_KEY) {
        current->command.command = CAB_CMD_ESTOP;
    }

    return;
}

uint16_t cabbus_get_loco_number(struct cabbus_cab* cab) {
    if (cab == NULL) return 0;
    return cab->loco_number;
}

struct cab_command* cabbus_cab_get_command(struct cabbus_cab* cab) {
    if (cab == NULL) return NULL;
    return &(cab->command);
}

void cabbus_cab_ask_question(struct cabbus_cab* cab, const char* message) {
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

uint8_t cabbus_cab_get_cab_number(struct cabbus_cab* cab) {
    if (cab == NULL) return 0;
    return cab->number;
}

void cabbus_cab_user_message(struct cabbus_cab* cab, const char* message) {
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

void cabbus_cab_set_user_data(struct cabbus_cab* cab, void* data) {
    if (cab == NULL) return;
    cab->user_data = data;
}

void* cabbus_cab_get_user_data(struct cabbus_cab* cab) {
    if (cab == NULL) return NULL;
    return cab->user_data;
}

int cabbus_cab_get_function(struct cabbus_cab* cab, uint8_t function) {
    if (cab == NULL) return 0;
    if (cab->functions & (0x01 << function)) {
        return 1;
    }

    return 0;
}
