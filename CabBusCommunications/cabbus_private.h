#ifndef CABBUS_PRIVATE_H
#define CABBUS_PRIVATE_H

#ifndef CABBUS_SECRET
#error "Super secret implementation details - don't use this header!"
#endif

#include <stdint.h>

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
#define HORN_KEY 0x49
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
#define HORN_KEY_RELEASE 0x5F
#define OPTION_KEY 0x69

#include "cab_commands.h"
#include "CabBus.h"

struct cabbus_cab {
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
    uint16_t current_selecting_number;
    //any other data that you want to associate with this cab.
    void* user_data;
};

struct cabbus_context;

void cabbus_cab_reset(struct cabbus_cab* cab);
void cabbus_cab_process_button_press(cab_write_fn write, struct cabbus_cab* cab, int keyByte);
void cabbus_cab_output_dirty_screens(cab_write_fn write, struct cabbus_cab* cab);

#endif /* CABBUS_PRIVATE_H */
