#include  "config.h"


#ifndef __UART_H 
#define __UART_H 

#define UART1_FIFO_LENGTH         8


uint8_t UART1_Init(uint32 BPS);
void UART1Putch(uint8 Data);   
void UART1Put_str(uint8 *Data, uint16 NByte);
void UART1Write_Str(uint8 *Data);


uint8 UART1GetFun(int *ncount);
uint8 UART1Getch(void);
uint8 UART1Get(void);

#endif






