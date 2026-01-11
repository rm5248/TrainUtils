/* SPDX-License-Identifier: GPL-2.0 */
#ifndef LCC_TRAIN_CONTROL_H
#define LCC_TRAIN_CONTROL_H

#include <stdint.h>

struct lcc_train_node;
struct lcc_context;
struct lcc_train_control_context;

enum LCC_Track_Protocol{
    LCC_TRACK_PROTOCOL_ANY = 0,
    LCC_TRACK_PROTOCOL_NATIVE_LCC = 1,
    LCC_TRACK_PROTOCOL_MFX = 2,
    LCC_TRACK_PROTOCOL_MARKLIN_MOTOROLA_ANY = 4,
    LCC_TRACK_PROTOCOL_MARKLIN_MOTOROLA_I = 5,
    LCC_TRACK_PROTOCOL_MARKLIN_MOTOROLA_II = 6,
    LCC_TRACK_PROTOCOL_MARKLIN_MOTOROLA_II_EXTRA = 7,

    /**
     * DCC addr space - OR with one of the LCC_TRACK_PROTOCOL_DCC_SPEED_xxx selections
     */
    LCC_TRACK_PROTOCOL_DCC_ADDR_SPACE_DEFAULT = 0x01 << 3,
    LCC_TRACK_PROTOCOL_DCC_ADDR_SPACE_FORCE_LONG = (0x01 << 3) | (0x01 << 2),

    /**
     * DCC Speed selections
     */
    LCC_TRACK_PROTOCOL_DCC_SPEED_DEFAULT = 0,
    LCC_TRACK_PROTOCOL_DCC_SPEED_14 = 1,
    LCC_TRACK_PROTOCOL_DCC_SPEED_28 = 2,
    LCC_TRACK_PROTOCOL_DCC_SPEED_128 = 3,
};

enum LCC_Train_Control_Query_state{
    LCC_TRAIN_CONTROL_NOT_QUERYING,
    LCC_TRAIN_CONTROL_QUERYING,
};

enum LCC_Train_Direction {
    LCC_TRAIN_DIRECTION_FORWARD,
    LCC_TRAIN_DIRECTION_BACKWARDS
};

/**
 * Return current time in milliseconds.  This does not need to be UNIX time, but can start at any point
 * (for example, when the application started).  All that matters is that this time is monotonically
 * increasing.
 */
typedef uint32_t (*lcc_time_query_fn)(void);

/**
 * Called once a train node has been properly allocated from the system.
 */
typedef void (*lcc_train_node_allocated)(struct lcc_train_control_context* ctx, struct lcc_train_node* train_node);

struct lcc_train_control_context* lcc_train_control_context_new(struct lcc_context* ctx);

int lcc_train_control_set_user_data(struct lcc_train_control_context* ctx, void* user_data);

void* lcc_train_control_user_data(struct lcc_train_control_context* ctx);

int lcc_train_control_set_time_query_function(struct lcc_train_control_context* ctx, lcc_time_query_fn query_fn);

/**
 * Find a train node with the given address.
 *
 * @param ctx
 * @param address
 * @param protocol_hints What protocol you want to search for.  Often you want to leave this as LCC_TRACK_PROTOCOL_ANY
 */
int lcc_train_control_find_train_by_address(struct lcc_train_control_context* ctx,
                                             uint16_t address,
                                             enum LCC_Track_Protocol protocol_hints);

/**
 * Allocate a train node for the given address
 *
 * @param ctx
 * @param address
 * @param protocol_hints
 * @param allocated_cb
 * @return
 */
int lcc_train_control_allocate_train_node_by_address(struct lcc_train_control_context* ctx,
                                             uint16_t address,
                                             enum LCC_Track_Protocol protocol_hints,
                                             lcc_train_node_allocated allocated_cb);

// --------------------------------------
// LCC Train Node functions
// --------------------------------------
int lcc_train_node_free(struct lcc_train_node* node);

int lcc_train_node_set_speed(struct lcc_train_node* train_node, uint8_t speed);

int lcc_train_node_set_direction(struct lcc_train_node* train_node, enum LCC_Train_Direction direction);

int lcc_train_node_set_function(struct lcc_train_node* train_node, uint8_t function, uint8_t value);


#endif /* LCC_TRAIN_CONTROL_H */
