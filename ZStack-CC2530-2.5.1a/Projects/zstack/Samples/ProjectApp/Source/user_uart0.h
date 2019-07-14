#ifndef USER_UART0_H
#define USER_UART0_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "ZComDef.h"

#define USER_UART_TX_BUFF_MAX  128
#define USER_UART_RX_BUFF_MAX  128
#define USER_UART_THRESHOLD   (USER_UART_RX_BUFF_MAX / 2)
#define USER_UART_IDLE_TIMEOUT 6

#define USER_UART_DEFAULT_OVERFLOW      FALSE
#define USER_UART_DEFAULT_THRESHOLD     USER_UART_THRESHOLD
#define USER_UART_DEFAULT_MAX_RX_BUFF   USER_UART_RX_BUFF_MAX
#define USER_UART_DEFAULT_MAX_TX_BUFF   USER_UART_TX_BUFF_MAX
#define USER_UART_DEFAULT_IDLE_TIMEOUT  USER_UART_IDLE_TIMEOUT
#define USER_UART_DEFAULT_PORT          HAL_UART_PORT_0

extern void USER_Uart0_Init( uint8 baudRate );

#ifdef __cplusplus
}
#endif

#endif /* USER_UART0_H */
#ifndef UART0_H
#define UART0_H
#ifdef __cplusplus
extern "C"
{
#endif
#include "hal_types.h"
 
#define UART0_RX_BUFF_MAX    60
#define UART0_TX_BUFF_MAX    60
#define UART0_THRESHOLD      (UART0_RX_BUFF_MAX / 2)
#define UART0_IDLE_TIMEOUT   6
 
extern uint8 UART0_RX_BUFF[UART0_RX_BUFF_MAX];//接收缓存区 
extern uint8 UART0_RX_STA;                    //接收状态标记	
extern uint8 UART0_RX_LEN;                    //接收数据长度
 
void Uart0_Init(uint8 baudRate);
void Uart0_Process( uint8 port, uint8 event );
void Uart0_Handle( void );
void Uart0_Reset( void );
 
#ifdef __cplusplus
}
#endif
#endif /* UART0_H */
