/**
  * This file contains some general bit checking utilities.
  * There are three macros, which follow the same format.  They are
  * CHECK_BIT, SET_BIT, and CLEAR_BIT.  Each takes the first argument
  * of a variable, and the second argument is the bit to operate on.
  * This bit number is zero-based.
  *
  * Example:
  *
  *  int x = 0;
  *  BIT_SET( x, 0 );
  *  BIT_SET( x, 2 );
  *  //At this point, bits 0 and 2 are set.  x is now 5
  *
  *  BIT_CLEAR( x, 0 ); //x is now 4
  *
  *  if( BIT_CHECK( x, 2 ) ){ //CHECK_BIT returns non-zero if the bit is set
  *    //code
  *  }
  *
  */

#ifndef RM_UTIL_H
#define RM_UTIL_H

#include <stdint.h>

#ifndef BIT
#define BIT(bit)			((uint64_t)0x01 << bit)
#endif

#define BIT_CHECK(number,bit)   (number & BIT(bit))
#define BIT_SET(number, bit)    (number |= BIT(bit))
#define BIT_CLEAR(number, bit)  (number &= (~BIT(bit)))

#ifndef ARRAY_SIZE
/**
 * Same as linux kernel macro
 */
#define ARRAY_SIZE(x)	(sizeof(x) / sizeof(x[0]))
#endif

#endif
