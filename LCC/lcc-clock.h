/* SPDX-License-Identifier: GPL-2.0 */
#ifndef LCC_CLOCK_H
#define LCC_CLOCK_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Some well-known clock types
 */
enum lcc_clock_type{
    LCC_CLOCK_TYPE_DEFAULT_FAST_CLOCK,
    LCC_CLOCK_TYPE_DEFAULT_REAL_TIME_CLOCK,
    LCC_CLOCK_TYPE_ALTERNATE1,
    LCC_CLOCK_TYPE_ALTERNATE2,
};

struct lcc_time{
    uint8_t hours;
    uint8_t minutes;
};

/**
 * Create an Event ID using the given clock type and the given time.  If the clock or time is invalid,
 * returns 0.
 *
 * @return
 */
uint64_t lcc_clock_report_time_eventid(enum lcc_clock_type, struct lcc_time);

/**
 * Check to see if the given Event ID is a 'Report Time' event.
 * This method only checks for the 4 well-known clock types.
 *
 * @param event_id
 * @return 1 if the event is a 'Report Time' event, 0 otherwise.
 */
int lcc_event_is_report_time_event(uint64_t event_id);

/**
 * Convert a given event to a 'Report Time' event.
 * This method only checks for the 4 well-known clock types.
 *
 * @param event_id The event to convert
 * @param type The clock type that this event is for.  May be NULL.
 * @param time The decoded time
 * @return LCC_OK on success, error code otherwise
 */
int lcc_event_to_report_time_event(uint64_t event_id, enum lcc_clock_type* type, struct lcc_time* time);

#ifdef __cplusplus
}
#endif

#endif // LCC_CLOCK_H
