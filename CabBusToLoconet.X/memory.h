/* 
 * File:   memory.h
 * Author: robert
 *
 * Created on January 16, 2015, 11:08 AM
 */

#ifndef MEMORY_H
#define	MEMORY_H

#include <inttypes.h>

#ifdef	__cplusplus
extern "C" {
#endif

    /**
     * Store the currently selected locomotive addr in nonvolatile memory
     * @param loco_addr
     */
    void memory_store_loco_addr( uint16_t loco_addr );

    /**
     * Get the last selected loco addr stored in memory.
     * @return
     */
    uint16_t memory_get_loco_addr();

#ifdef	__cplusplus
}
#endif

#endif	/* MEMORY_H */

