#include "RS485Comm.h"

void SendDataBuffer(UART_MODULE uart_num, const char *buffer, UINT32 size) {

    if (uart_num == UART1) {
        //Make sure we can transmit on UART1
        PORTAbits.RA0 = 1;
    } else {
        PORTBbits.RB5 = 1;
    }

    while (size) {
        while (!UARTTransmitterIsReady(uart_num))
            ;

        UARTSendDataByte(uart_num, *buffer);

        buffer++;
        size--;
    }

    while (!UARTTransmissionHasCompleted(uart_num))
        ;

    if (uart_num == UART1) {
        //clear UART1 from transmit
        PORTAbits.RA0 = 0;
    } else {
        PORTBbits.RB5 = 0;
    }
}
