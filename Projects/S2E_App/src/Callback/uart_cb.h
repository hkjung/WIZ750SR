#ifndef __UART_CB_H__
#define __UART_CB_H__

#include <stdint.h>

//#define UART_SEND_BYTE(ch)  UART_SEND_BYTE_CB(ch)
//#define UART_RECV_BYTE()    UART_RECV_BYTE_CB()

//uint8_t UART_SEND_BYTE(uint8_t ch);
//uint8_t UART_RECV_BYTE(void);

uint8_t (*UART_SEND_BYTE)(uint8_t ch);
uint8_t (*UART_RECV_BYTE)(void);

void reg_uart_cbfunc(uint8_t(*uart_send_byte)(uint8_t ch), uint8_t(*uart_recv_byte)(void));

#endif
