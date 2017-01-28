/* 
 * File:   config.h
 * Author: Rob
 *
 * Created on November 30, 2013, 10:09 AM
 */

/**
 * This file contains the general (static) configuration of the microcontroller
 */
#ifndef CONFIG_H
#define	CONFIG_H

#ifdef	__cplusplus
extern "C" {
#endif

#include <p32xxxx.h>
#include <plib.h>

#define SYS_FREQ (40000000L)

#define	GetPeripheralClock()		(SYS_FREQ/(1 << OSCCONbits.PBDIV))
#define	GetInstructionClock()		(SYS_FREQ)

#ifdef	__cplusplus
}
#endif

#endif	/* CONFIG_H */

