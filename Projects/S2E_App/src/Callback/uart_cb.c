/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include "common.h"
#include "W7500x_uart.h"
#include "W7500x_board.h"
#include "uart_cb.h"

/* Private functions prototypes ----------------------------------------------*/

/* The default callback function */
uint8_t default_uart_send_byte(uint8_t ch);
uint8_t default_uart_recv_byte(void);
uint8_t uart0_send_byte(uint8_t ch);
uint8_t uart0_recv_byte(void);
uint8_t uart1_send_byte(uint8_t ch);
uint8_t uart1_recv_byte(void);

/* UART Callback handler */
uint8_t (*UART_SEND_BYTE)(uint8_t ch) = default_uart_send_byte;  /* handler to be called when the UART is initialized */
uint8_t (*UART_RECV_BYTE)(void) = default_uart_recv_byte;     /* handler to be called when the IP address from DHCP server is updated */


/* Public functions ----------------------------------------------------------*/

void reg_uart_cbfunc(uint8_t(*uart_send_byte)(uint8_t ch), uint8_t(*uart_recv_byte)(void))
{
	UART_SEND_BYTE = default_uart_send_byte;
	UART_RECV_BYTE = default_uart_recv_byte;

	if(uart_send_byte) UART_SEND_BYTE = uart_send_byte;
	if(uart_recv_byte) UART_RECV_BYTE = uart_recv_byte;
}

/* UART Callback handler */
//uint8_t (*UART_SEND_BYTE_CB)(uint8_t ch) = default_uart_send_byte;  /* handler to be called when the UART is initialized */
//uint8_t (*UART_RECV_BYTE_CB)(void) = default_uart_recv_byte;     /* handler to be called when the IP address from DHCP server is updated */

/* Public functions ----------------------------------------------------------*/
/*
void reg_uart_cbfunc(uint8_t(*uart_send_byte)(uint8_t ch), uint8_t(*uart_recv_byte)(void))
{
	UART_SEND_BYTE_CB = default_uart_send_byte;
	UART_RECV_BYTE_CB = default_uart_recv_byte;

	if(uart_send_byte) UART_SEND_BYTE_CB = uart_send_byte;
	if(uart_recv_byte) UART_RECV_BYTE_CB = uart_recv_byte;
}
*/

/* Private functions ---------------------------------------------------------*/

uint8_t default_uart_send_byte(uint8_t ch)
{
	S_UartPutc(ch);
	return ch;
}

uint8_t default_uart_recv_byte(void)
{
	return S_UartGetc();
}

uint8_t uart0_send_byte(uint8_t ch)
{
	UartPutc(UART0,ch);
	return ch;
}

uint8_t uart0_recv_byte(void)
{
	return UartGetc(UART0);
}

uint8_t uart1_send_byte(uint8_t ch)
{
	UartPutc(UART0,ch);
	return ch;
}

uint8_t uart1_recv_byte(void)
{
	return UartGetc(UART0);
}
