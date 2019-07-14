#include "user_uart0.h"
#include "hal_uart.h"
#include "string.h"
//#include "user_printf.h"
#include "AF.h"
#include "ProjectApp.h"
#include "ZDProfile.h"
#include "user_printf.h"

uint8 UART0_RX_BUFF[UART0_RX_BUFF_MAX];//���ջ�����
uint8 UART0_RX_STA = 0;                //����״̬���
uint8 UART0_RX_LEN = 0;                //�������ݳ���
//extern cId_t ProjectApp_ClusterList[PROJECTAPP_MAX_CLUSTERS];
extern const cId_t ProjectApp_ClusterList[PROJECTAPP_MAX_CLUSTERS];
extern byte ProjectApp_TransID;  // This is the unique message ID (counter)

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
  uartConfig.callBackFunc         = Uart0_Process;

  /* Start UART */
  HalUARTOpen (USER_UART_DEFAULT_PORT, &uartConfig);
}

void Uart0_Process( uint8 port, uint8 event )
{
  uint8 Res;
  (void)event;  // Intentionally unreferenced parameter
 
  while (Hal_UART_RxBufLen(port))
  {
    HalUARTRead (port, &Res, 1);//��ȡһ���ֽ�
    UART0_RX_BUFF[UART0_RX_STA&0x3F]=Res ;
    UART0_RX_STA++;
 
    if((UART0_RX_STA&0x80)==0)//����δ���
    {
      if(UART0_RX_STA&0x40)//ǰһ�����յ������ݽ������ġ�ǰһ�롱
      {
        if     (Res==0x0A)UART0_RX_STA |= 0x80;//���������
        else if(Res!=0x0D)UART0_RX_STA &= 0xBF;//���ǽ���������ǰ�����������־��0
      }
      else
      {
        if(Res==0x0D)UART0_RX_STA |= 0x40;
      }
    }
 
    if((UART0_RX_STA&0x80)==0x80)
    {
      UART0_RX_LEN = (UART0_RX_STA&0x3F)-2;
      Uart0_Handle();
      Uart0_Reset();
      break;
    }
  }
}

static void ProjectApp_SendBindcast( void )
{
  char theMessageData[ ] = "Bind data\r\n";
  
  afAddrType_t ProjectApp_DstAddr;
  ProjectApp_DstAddr.addrMode       = (afAddrMode_t)AddrNotPresent;
  ProjectApp_DstAddr.endPoint       = 0;
  ProjectApp_DstAddr.addr.shortAddr = 0;
 
  AF_DataRequest( &ProjectApp_DstAddr,
                  &ProjectApp_epDesc,
                  PROJECTAPP_CLUSTERID,
                  (byte)osal_strlen( theMessageData ) + 1,
                  (byte *)&theMessageData,
                  &ProjectApp_TransID,
                  AF_DISCV_ROUTE,
                  AF_DEFAULT_RADIUS
                );
}

void Uart0_Handle(void)
{
  if(strstr((const char*)UART0_RX_BUFF, "request binding"))
  {
    printf("Bind start!\r\n");
    zAddrType_t dstAddr;
    dstAddr.addrMode = Addr16Bit;
    dstAddr.addr.shortAddr = 0x0000; // Coordinator
    ZDP_EndDeviceBindReq( &dstAddr, NLME_GetShortAddr(),
                          ProjectApp_epDesc.endPoint,
                          PROJECTAPP_PROFID,
                          PROJECTAPP_MAX_CLUSTERS, (cId_t *)ProjectApp_ClusterList,
                          PROJECTAPP_MAX_CLUSTERS, (cId_t *)ProjectApp_ClusterList,
                          FALSE );

  }
  else if(strstr((const char*)UART0_RX_BUFF,"send binding"))
  {
      ProjectApp_SendBindcast();
  }
}

void Uart0_Reset(void)
{
  memset(UART0_RX_BUFF,0,UART0_RX_BUFF_MAX);
  UART0_RX_STA = 0;
}
