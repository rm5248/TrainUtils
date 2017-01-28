#include "Delay.h"

#include <p32xxxx.h>
#include <plib.h>

#include "config.h"

#define CORE_TIMER_FREQUENCY            (SYS_FREQ/2)
#define CORE_TIMER_MILLISECONDS         (CORE_TIMER_FREQUENCY/1000)
#define CORE_TIMER_MICROSECONDS         (CORE_TIMER_FREQUENCY/1000000)

//*****************************************************************************
// DelayUs creates a delay of given microseconds using the Core Timer
//
// 1 million micro seconds in a second!

void DelayUs(unsigned int delay) {
   UINT32   DelayStartTime;

   DelayStartTime = ReadCoreTimer();
   while((ReadCoreTimer() - DelayStartTime) < (delay * CORE_TIMER_MICROSECONDS));
   
}

//*****************************************************************************
// DelayMs creates a delay of given miliseconds using the Core Timer
//

void DelayMs(unsigned short delay) {
   UINT32   DelayStartTime;

   DelayStartTime = ReadCoreTimer();
   while((ReadCoreTimer() - DelayStartTime) < (delay * CORE_TIMER_MILLISECONDS));
}
