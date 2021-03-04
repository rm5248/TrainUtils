#ifndef CAB_COMMANDS_H
#define CAB_COMMANDS_H

#include <stdint.h>

#define CAB_SWITCH_NORMAL 1
#define CAB_SWITCH_REVERSE 2

/**
 * Commands that are sent by the cab bus.
 */

enum CabCommands {
    /* No command */
    CAB_CMD_NONE,
    /* Select a locomotive */
    CAB_CMD_SEL_LOCO,
    /* Throw a switch */
    CAB_CMD_SWITCH,
    /* Throw a macro */
    CAB_CMD_MACRO,
    /* Response back from user */
    CAB_CMD_RESPONSE,
    /* Set speed */
    CAB_CMD_SPEED,
    /* Set direction */
    CAB_CMD_DIRECTION,
    /* ESTOP.  No args */
    CAB_CMD_ESTOP,
    /* Set a function on/off */
    CAB_CMD_FUNCTION,
    /* Unselect/release a locomotive(addr 0) */
    CAB_CMD_UNSELECT_LOCO,
};



struct cab_select_loco{
    uint8_t flags; /* Long addr = 1, short addr = 0 */
    uint16_t address; /* address of the locomotive to select */
};

struct cab_speed {
    uint8_t speed; /* lower 7 bits = speed */
};

/**
 * A response from the user regarding a question that was asked of them.
 */
struct user_response {
    uint8_t response; /* 0 = false(no), 1 = true(yes) */
};

struct loco_direction {
    uint8_t direction; /* 1 = forward, 0 = reverse */
};

struct loco_function{
    uint8_t function_number;
    uint8_t onoff;
};

struct cab_command_switch {
    uint16_t switch_number;
    uint8_t normal_rev;
};

struct cab_command{
    uint8_t command; /* one of the CAB_CMD macros */
    union {
        struct cab_select_loco sel_loco;
        struct cab_speed  speed;
        struct user_response response;
        struct loco_direction direction;
        struct loco_function function;
        struct cab_command_switch switch_state;
    };
};

const char* cabbus_command_to_string( int command );

#endif /* CAB_COMMANDS_H */
