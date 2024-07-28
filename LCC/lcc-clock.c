/* SPDX-License-Identifier: GPL-2.0 */
#include "lcc-clock.h"

uint64_t lcc_clock_report_time_eventid(enum lcc_clock_type type, struct lcc_time time){
    uint64_t ret = 0;

    switch(type){
    case LCC_CLOCK_TYPE_DEFAULT_FAST_CLOCK:
        ret = 0x010100000100ll;
        break;
    case LCC_CLOCK_TYPE_DEFAULT_REAL_TIME_CLOCK:
        ret = 0x010100000101ll;
        break;
    case LCC_CLOCK_TYPE_ALTERNATE1:
        ret = 0x010100000102ll;
        break;
    case LCC_CLOCK_TYPE_ALTERNATE2:
        ret = 0x010100000103ll;
        break;
    }

    // Make sure our clock is good
    if(ret == 0){
        return ret;
    }

    // Check to make sure that the time is sane.
    if(time.hours > 24 || time.minutes > 59){
        return 0;
    }

    ret = ret << 16;

    ret |= (time.hours << 8);
    ret |= time.minutes;

    return ret;
}
