/* SPDX-License-Identifier: GPL-2.0 */
#include "lcc-clock.h"
#include "lcc-common.h"
#include "lcc-common-internal.h"

#define DEFAULT_FAST_CLOCK 0x0101000001000000ll
#define DEFAULT_REAL_TIME_CLOCK 0x0101000001010000ll
#define ALTERNATE_1 0x0101000001020000ll
#define ALTERNATE_2 0x0101000001030000ll

uint64_t lcc_clock_report_time_eventid(enum lcc_clock_type type, struct lcc_time time){
    uint64_t ret = 0;

    switch(type){
    case LCC_CLOCK_TYPE_DEFAULT_FAST_CLOCK:
        ret = DEFAULT_FAST_CLOCK;
        break;
    case LCC_CLOCK_TYPE_DEFAULT_REAL_TIME_CLOCK:
        ret = DEFAULT_REAL_TIME_CLOCK;
        break;
    case LCC_CLOCK_TYPE_ALTERNATE1:
        ret = ALTERNATE_1;
        break;
    case LCC_CLOCK_TYPE_ALTERNATE2:
        ret = ALTERNATE_2;
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

    ret |= (time.hours << 8);
    ret |= time.minutes;

    return ret;
}

int lcc_event_is_report_time_event(uint64_t event_id){
    uint64_t event_id_masked = event_id & 0xFFFFFFFFFFFF0000ll;
    int clock_valid = 0;
    int time_valid = 1;

    if(event_id_masked == DEFAULT_FAST_CLOCK){
        clock_valid = 1;
    }else if(event_id_masked == DEFAULT_REAL_TIME_CLOCK){
        clock_valid = 1;
    }else if(event_id_masked == ALTERNATE_1){
        clock_valid = 1;
    }else if(event_id_masked == ALTERNATE_2){
        clock_valid = 1;
    }

    uint8_t event_data[8];
    lcc_event_id_to_array(event_id, event_data);
    if(event_data[6] > 0x17 ||
            event_data[7] > 59){
        time_valid = 0;
    }

    if(clock_valid && time_valid){
        return 1;
    }

    return 0;
}

int lcc_event_to_report_time_event(uint64_t event_id, enum lcc_clock_type* type, struct lcc_time* time){
    if(!lcc_event_is_report_time_event(event_id)){
        return LCC_ERROR_EVENT_NOT_REPORT_TIME;
    }

    if(type){
        uint64_t event_id_masked = event_id & 0xFFFFFFFFFFFF0000ll;

        if(event_id_masked == DEFAULT_FAST_CLOCK){
            *type = LCC_CLOCK_TYPE_DEFAULT_FAST_CLOCK;
        }else if(event_id_masked == DEFAULT_REAL_TIME_CLOCK){
            *type = LCC_CLOCK_TYPE_DEFAULT_REAL_TIME_CLOCK;
        }else if(event_id_masked == ALTERNATE_1){
            *type = LCC_CLOCK_TYPE_ALTERNATE1;
        }else if(event_id_masked == ALTERNATE_2){
            *type = LCC_CLOCK_TYPE_ALTERNATE2;
        }
    }

    if(time){
        uint8_t event_data[8];
        lcc_event_id_to_array(event_id, event_data);
        time->hours = event_data[6];
        time->minutes = event_data[7];
    }

    return LCC_OK;
}
