/* 
 * File:   Delay.h
 * Author: Rob
 *
 * Created on November 30, 2013, 10:08 AM
 */

#ifndef DELAY_H
#define	DELAY_H

#ifdef	__cplusplus
extern "C" {
#endif

    /**
     * Delay a given number of milliseconds
     *
     * @param delay Number of milliseconds to delay
     */
    void DelayMs(unsigned short delay);

    /**
     * Delay a given number of microseconds
     *
     * @param delay
     */
    void DelayUs(unsigned int delay);


#ifdef	__cplusplus
}
#endif

#endif	/* DELAY_H */

