#include "OSAL.h"
#include "AF.h"
#include "ZDApp.h"
#include "ZDObject.h"
#include "ZDProfile.h"

#include "ProjectApp.h"
#include "DebugTrace.h"

#if !defined( WIN32 )
  #include "OnBoard.h"
#endif

/* HAL */
#include "hal_lcd.h"
#include "hal_led.h"
#include "hal_key.h"
#include "hal_uart.h"

/* RTOS */
#if defined( IAR_ARMCM3_LM )
#include "RTOS_App.h"
#endif  

#include "user_uart0.h"
#include "user_printf.h"
#include "user_api.h"
/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * CONSTANTS
 */

/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */
// This list should be filled with Application specific Cluster IDs.
const cId_t ProjectApp_ClusterList[PROJECTAPP_MAX_CLUSTERS] =
{
  PROJECTAPP_CLUSTERID
};

const SimpleDescriptionFormat_t ProjectApp_SimpleDesc =
{
  PROJECTAPP_ENDPOINT,              //  int Endpoint;
  PROJECTAPP_PROFID,                //  uint16 AppProfId[2];
  PROJECTAPP_DEVICEID,              //  uint16 AppDeviceId[2];
  PROJECTAPP_DEVICE_VERSION,        //  int   AppDevVer:4;
  PROJECTAPP_FLAGS,                 //  int   AppFlags:4;
  PROJECTAPP_MAX_CLUSTERS,          //  byte  AppNumInClusters;
  (cId_t *)ProjectApp_ClusterList,  //  byte *pAppInClusterList;
  PROJECTAPP_MAX_CLUSTERS,          //  byte  AppNumInClusters;
  (cId_t *)ProjectApp_ClusterList   //  byte *pAppInClusterList;
};

// This is the Endpoint/Interface description.  It is defined here, but
// filled-in in ProjectApp_Init().  Another way to go would be to fill
// in the structure here and make it a "const" (in code space).  The
// way it's defined in this sample app it is define in RAM.
endPointDesc_t ProjectApp_epDesc;

/*********************************************************************
 * EXTERNAL VARIABLES
 */

/*********************************************************************
 * EXTERNAL FUNCTIONS
 */

/*********************************************************************
 * LOCAL VARIABLES
 */
byte ProjectApp_TaskID;   // Task ID for internal task/event processing
                          // This variable will be received when
                          // ProjectApp_Init() is called.
devStates_t ProjectApp_NwkState;


byte ProjectApp_TransID;  // This is the unique message ID (counter)

afAddrType_t ProjectApp_DstAddr;

/*********************************************************************
 * LOCAL FUNCTIONS
 */

static void ProjectApp_HandleKeys( byte shift, byte keys );
static void ProjectApp_SendTheMessage( void );
static void ProjectApp_ProcessZDOMsgs( zdoIncomingMsg_t *inMsg );
static void ProjectApp_SendBindcast( void );
static void ProjectApp_MessageMSGCB( afIncomingMSGPacket_t *pkt );
#if defined( IAR_ARMCM3_LM )
static void ProjectApp_ProcessRtosMessage( void );
#endif

/*********************************************************************
 * NETWORK LAYER CALLBACKS
 */

/*********************************************************************
 * PUBLIC FUNCTIONS
 */

/*********************************************************************
 * @fn      ProjectApp_Init
 *
 * @brief   Initialization function for the Project App Task.
 *          This is called during initialization and should contain
 *          any application specific initialization (ie. hardware
 *          initialization/setup, table initialization, power up
 *          notificaiton ... ).
 *
 * @param   task_id - the ID assigned by OSAL.  This ID should be
 *                    used to send messages and set timers.
 *
 * @return  none
 */
void ProjectApp_Init( uint8 task_id )
{
  ProjectApp_TaskID = task_id;
  ProjectApp_NwkState = DEV_INIT;
  ProjectApp_TransID = 0;

  // Device hardware initialization can be added here or in main() (Zmain.c).
  // If the hardware is application specific - add it here.
  // If the hardware is other parts of the device add it in main().

  ProjectApp_DstAddr.addrMode = (afAddrMode_t)AddrNotPresent;
  ProjectApp_DstAddr.endPoint = 0;
  ProjectApp_DstAddr.addr.shortAddr = 0;

  // Fill out the endpoint description.
  ProjectApp_epDesc.endPoint = PROJECTAPP_ENDPOINT;
  ProjectApp_epDesc.task_id = &ProjectApp_TaskID;
  ProjectApp_epDesc.simpleDesc
            = (SimpleDescriptionFormat_t *)&ProjectApp_SimpleDesc;
  ProjectApp_epDesc.latencyReq = noLatencyReqs;

  // Register the endpoint description with the AF
  afRegister( &ProjectApp_epDesc );

  // Register for all key events - This app will handle all key events
  RegisterForKeys( ProjectApp_TaskID );

  // Update the display
#if defined ( LCD_SUPPORTED )
  HalLcdWriteString( "ProjectApp", HAL_LCD_LINE_1 );
#endif

  ZDO_RegisterForZDOMsg( ProjectApp_TaskID, End_Device_Bind_rsp );
  ZDO_RegisterForZDOMsg( ProjectApp_TaskID, Match_Desc_rsp );

#if defined( IAR_ARMCM3_LM )
  // Register this task with RTOS task initiator
  RTOS_RegisterApp( task_id, PROJECTAPP_RTOS_MSG_EVT );
#endif
  
   USER_Uart0_Init(HAL_UART_BR_115200);
   
ZDO_RegisterForZDOMsg( ProjectApp_TaskID, End_Device_Bind_rsp );
}

/*********************************************************************
 * @fn      ProjectApp_ProcessEvent
 *
 * @brief   Project Application Task event processor.  This function
 *          is called to process all events for the task.  Events
 *          include timers, messages and any other user defined events.
 *
 * @param   task_id  - The OSAL assigned task ID.
 * @param   events - events to process.  This is a bit map and can
 *                   contain more than one event.
 *
 * @return  none
 */
uint16 ProjectApp_ProcessEvent( uint8 task_id, uint16 events )
{
  afIncomingMSGPacket_t *MSGpkt;
  afDataConfirm_t *afDataConfirm;

  // Data Confirmation message fields
  byte sentEP;
  ZStatus_t sentStatus;
  byte sentTransID;       // This should match the value sent
  (void)task_id;  // Intentionally unreferenced parameter

  if ( events & SYS_EVENT_MSG )
  {
    MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive( ProjectApp_TaskID );
    while ( MSGpkt )
    {
      switch ( MSGpkt->hdr.event )
      {
        case ZDO_CB_MSG:
          ProjectApp_ProcessZDOMsgs( (zdoIncomingMsg_t *)MSGpkt );
          break;

        case KEY_CHANGE:
          ProjectApp_HandleKeys( ((keyChange_t *)MSGpkt)->state, ((keyChange_t *)MSGpkt)->keys );
          break;

        case AF_DATA_CONFIRM_CMD:
          // This message is received as a confirmation of a data packet sent.
          // The status is of ZStatus_t type [defined in ZComDef.h]
          // The message fields are defined in AF.h
          afDataConfirm = (afDataConfirm_t *)MSGpkt;
          sentEP = afDataConfirm->endpoint;
          sentStatus = afDataConfirm->hdr.status;
          sentTransID = afDataConfirm->transID;
          (void)sentEP;
          (void)sentTransID;

          // Action taken when confirmation is received.
          if ( sentStatus != ZSuccess )
          {
            // The data wasn't delivered -- Do something
          }
          break;

        case AF_INCOMING_MSG_CMD:
           ProjectApp_MessageMSGCB( MSGpkt );
          break;
          
        case ZDO_STATE_CHANGE:
           user_state_change((devStates_t)(MSGpkt->hdr.status));
          break;

        default:
          break;
      }

      // Release the memory
      osal_msg_deallocate( (uint8 *)MSGpkt );

      // Next
      MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive( ProjectApp_TaskID );
    }

    // return unprocessed events
    return (events ^ SYS_EVENT_MSG);
  }

  // Send a message out - This event is generated by a timer
  //  (setup in ProjectApp_Init()).
  if ( events & PROJECTAPP_SEND_MSG_EVT )
  {
    // Send "the" message
    ProjectApp_SendTheMessage();

    // Setup to send message again
    osal_start_timerEx( ProjectApp_TaskID,
                        PROJECTAPP_SEND_MSG_EVT,
                        PROJECTAPP_SEND_MSG_TIMEOUT );

    // return unprocessed events
    return (events ^ PROJECTAPP_SEND_MSG_EVT);
  }

  
#if defined( IAR_ARMCM3_LM )
  // Receive a message from the RTOS queue
  if ( events & PROJECTAPP_RTOS_MSG_EVT )
  {
    // Process message from RTOS queue
    ProjectApp_ProcessRtosMessage();

    // return unprocessed events
    return (events ^ PROJECTAPP_RTOS_MSG_EVT);
  }
#endif

  // Discard unknown events
  return 0;
}


/*********************************************************************
 * @fn      ProjectApp_HandleKeys
 *
 * @brief   Handles all key events for this device.
 *
 * @param   shift - true if in shift/alt.
 * @param   keys - bit field for key events. Valid entries:
 *                 HAL_KEY_SW_4
 *                 HAL_KEY_SW_3
 *                 HAL_KEY_SW_2
 *                 HAL_KEY_SW_1
 *
 * @return  none
 */
static void ProjectApp_HandleKeys( uint8 shift, uint8 keys )
{
  zAddrType_t dstAddr;

  // Shift is used to make each button/switch dual purpose.
  if ( shift )
  {
    if ( keys & HAL_KEY_SW_1 )
    {
    }
    if ( keys & HAL_KEY_SW_2 )
    {
    }
    if ( keys & HAL_KEY_SW_3 )
    {
    }
    if ( keys & HAL_KEY_SW_4 )
    {
    }
  }
  else
  {
    if ( keys & HAL_KEY_SW_1 )
    {
    ProjectApp_SendBindcast();
    }

#if defined( SWITCH1_BIND )
      // we can use SW1 to simulate SW2 for devices that only have one switch,
      keys |= HAL_KEY_SW_2;
#elif defined( SWITCH1_MATCH )
      // or use SW1 to simulate SW4 for devices that only have one switch
      keys |= HAL_KEY_SW_4;
#endif
    }

    if ( keys & HAL_KEY_SW_2 )
    {
      HalLedSet ( HAL_LED_4, HAL_LED_MODE_OFF );

      // Initiate an End Device Bind Request for the mandatory endpoint
      dstAddr.addrMode = Addr16Bit;
      dstAddr.addr.shortAddr = 0x0000; // Coordinator
      ZDP_EndDeviceBindReq( &dstAddr, NLME_GetShortAddr(),
                            ProjectApp_epDesc.endPoint,
                            PROJECTAPP_PROFID,
                            PROJECTAPP_MAX_CLUSTERS, (cId_t *)ProjectApp_ClusterList,
                            PROJECTAPP_MAX_CLUSTERS, (cId_t *)ProjectApp_ClusterList,
                            FALSE );
    }

    if ( keys & HAL_KEY_SW_3 )
    {
    }

    if ( keys & HAL_KEY_SW_4 )
    {
      HalLedSet ( HAL_LED_4, HAL_LED_MODE_OFF );
      // Initiate a Match Description Request (Service Discovery)
      dstAddr.addrMode = AddrBroadcast;
      dstAddr.addr.shortAddr = NWK_BROADCAST_SHORTADDR;
      ZDP_MatchDescReq( &dstAddr, NWK_BROADCAST_SHORTADDR,
                        PROJECTAPP_PROFID,
                        PROJECTAPP_MAX_CLUSTERS, (cId_t *)ProjectApp_ClusterList,
                        PROJECTAPP_MAX_CLUSTERS, (cId_t *)ProjectApp_ClusterList,
                        FALSE );
    }
  }


static void ProjectApp_ProcessZDOMsgs( zdoIncomingMsg_t *inMsg )//每次“绑定”状态发生改变，均会调用此函数
{
  switch ( inMsg->clusterID )
  {
    case End_Device_Bind_rsp:
      if ( ZDO_ParseBindRsp( inMsg ) == ZSuccess )
      {
        printf("Bind success!\r\n");
      }
      else
      {
        printf("Bind failure!\r\n");
      }
      break;
  }
}

static void ProjectApp_SendBindcast( void )
{
  char theMessageData[ ] = "Bind data\r\n";
 
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

/*********************************************************************
 * LOCAL FUNCTIONS
 */

/*********************************************************************
 * @fn      ProjectApp_MessageMSGCB
 *
 * @brief   Data message processor callback.  This function processes
 *          any incoming data - probably from other devices.  So, based
 *          on cluster ID, perform the intended action.
 *
 * @param   none
 *
 * @return  none
 */
static void ProjectApp_MessageMSGCB( afIncomingMSGPacket_t *pkt )
{
  switch ( pkt->clusterId )
  {
    case PROJECTAPP_CLUSTERID:
      printf("%s\r\n", pkt->cmd.Data);
      break;
  }
}

/*********************************************************************
 * @fn      ProjectApp_SendTheMessage
 *
 * @brief   Send "the" message.
 *
 * @param   none
 *
 * @return  none
 */
static void ProjectApp_SendTheMessage( void )
{
  char theMessageData[] = "Hello World";

  if ( AF_DataRequest( &ProjectApp_DstAddr, &ProjectApp_epDesc,
                       PROJECTAPP_CLUSTERID,
                       (byte)osal_strlen( theMessageData ) + 1,
                       (byte *)&theMessageData,
                       &ProjectApp_TransID,
                       AF_DISCV_ROUTE, AF_DEFAULT_RADIUS ) == afStatus_SUCCESS )
  {
    // Successfully requested to be sent.
  }
  else
  {
    // Error occurred in request to send.
  }
}

#if defined( IAR_ARMCM3_LM )
/*********************************************************************
 * @fn      ProjectApp_ProcessRtosMessage
 *
 * @brief   Receive message from RTOS queue, send response back.
 *
 * @param   none
 *
 * @return  none
 */
static void ProjectApp_ProcessRtosMessage( void )
{
  osalQueue_t inMsg;

  if ( osal_queue_receive( OsalQueue, &inMsg, 0 ) == pdPASS )
  {
    uint8 cmndId = inMsg.cmnd;
    uint32 counter = osal_build_uint32( inMsg.cbuf, 4 );

    switch ( cmndId )
    {
      case CMD_INCR:
        counter += 1;  /* Increment the incoming counter */
                       /* Intentionally fall through next case */

      case CMD_ECHO:
      {
        userQueue_t outMsg;

        outMsg.resp = RSP_CODE | cmndId;  /* Response ID */
        osal_buffer_uint32( outMsg.rbuf, counter );    /* Increment counter */
        osal_queue_send( UserQueue1, &outMsg, 0 );  /* Send back to UserTask */
        break;
      }
      
      default:
        break;  /* Ignore unknown command */    
    }
  }
}
#endif

/*********************************************************************
 */
