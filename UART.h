

/*
 * UART.h - Header file for UART.c
 *
 */

#ifndef UART_H_
#define UART_H_




/*******************************************************************************************
* Public Function Prototypes
*******************************************************************************************/
typedef struct{
    INT16U tsample;
    OS_SEM flag;
}DATA;

static DATA Drive;

void UARTInit(void);



#endif /* UART_H_ */
