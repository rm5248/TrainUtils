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
  *  SET_BIT( x, 0 );
  *  SET_BIT( x, 2 );
  *  //At this point, bits 0 and 2 are set.  x is now 5
  *
  *  CLEAR_BIT( x, 0 ); //x is now 4
  *
  *  if( CHECK_BIT( x, 2 ) ){ //CHECK_BIT returns non-zero if the bit is set
  *    //code
  *  }
  *
  */

#ifndef RM_UTIL_H
#define RM_UTIL_H

#define CHECK_BIT(number,bit)   (number & (0x1 << bit))
#define SET_BIT(number, bit)    (number |= (0x1 << bit))
#define CLEAR_BIT(number, bit)  (number &= (~(0x1 << bit)))

#endif