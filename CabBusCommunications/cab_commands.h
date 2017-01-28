/**
 * Commands that are sent by the cab bus.
 */

/* No command */
#define CAB_CMD_NONE 0x00
/* Select a locomotive */
#define CAB_CMD_SEL_LOCO 0x01
/* Throw a switch */
#define CAB_CMD_SWITCH 0x02
/* Throw a macro */
#define CAB_CMD_MACRO 0x03
/* Response back from user */
#define CAB_CMD_RESPONSE 0x04
/* Set speed */
#define CAB_CMD_SPEED 0x05
/* Set direction */
#define CAB_CMD_DIRECTION 0x06
/* ESTOP.  No args */
#define CAB_CMD_ESTOP 0x07

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

struct cab_command{
    uint8_t command; /* one of the CAB_CMD macros */
    union {
        struct cab_select_loco sel_loco;
        struct cab_speed  speed;
        struct user_response response;
        struct loco_direction direction;
    };
};
