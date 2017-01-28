/* 
 * File:   RS485Comm.h
 * Author: Rob
 *
 * Created on November 29, 2013, 4:07 PM
 */

#ifndef RS485COMM_H
#define	RS485COMM_H

#ifdef	__cplusplus
extern "C" {
#endif

#include <p32xxxx.h>

#include <plib.h>

void SendDataBuffer(UART_MODULE uart_num, const char *buffer, UINT32 size);


#ifdef	__cplusplus
}
#endif

#endif	/* RS485COMM_H */

