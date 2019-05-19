#include "user_uart0.h"
#include "hal_uart.h"

void USER_Uart0_Init( uint8 baudRate )
{
  halUARTCfg_t uartConfig;

  /* UART Configuration */
  uartConfig.configured           = TRUE;
  uartConfig.baudRate             = baudRate;
  uartConfig.flowControl          = USER_UART_DEFAULT_OVERFLOW;
  uartConfig.flowControlThreshold = USER_UART_DEFAULT_THRESHOLD;
  uartConfig.rx.maxBufSize        = USER_UART_DEFAULT_MAX_RX_BUFF;
  uartConfig.tx.maxBufSize        = USER_UART_DEFAULT_MAX_TX_BUFF;
  uartConfig.idleTimeout          = USER_UART_DEFAULT_IDLE_TIMEOUT;
  uartConfig.intEnable            = TRUE;
  uartConfig.callBackFunc         = NULL;

  /* Start UART */
  HalUARTOpen (USER_UART_DEFAULT_PORT, &uartConfig);
}
