#include "user_api.h"
#include "ProjectApp.h"
#include "user_printf.h"
#include "string.h"
const uint8* devStates_str[11]=
{
  "DEV_HOLD",               // Initialized - not started automatically
  "DEV_INIT",               // Initialized - not connected to anything
  "DEV_NWK_DISC",           // Discovering PAN's to join
  "DEV_NWK_JOINING",        // Joining a PAN
  "DEV_NWK_REJOIN",         // ReJoining a PAN, only for end devices
  "DEV_END_DEVICE_UNAUTH",  // Joined but not yet authenticated by trust center
  "DEV_END_DEVICE",         // Started as device after authentication
  "DEV_ROUTER",             // Device joined, authenticated and is a router
  "DEV_COORD_STARTING",     // Started as Zigbee Coordinator
  "DEV_ZB_COORD",           // Started as Zigbee Coordinator
  "DEV_NWK_ORPHAN"          // Device has lost information about its parent..
};

void user_show_info(void)
{
  uint8 *MacAddr = 0;

  printf("Channel:%02X\r\n", _NIB.nwkLogicalChannel);
  printf("NwkPANID:%04X\r\n",_NIB.nwkPanId);

  NLME_GetCoordExtAddr(MacAddr);
  printf("Fath_Nwk:%04X  ",NLME_GetCoordShortAddr());
  printf("Fath_Mac:%04X%04X%04X%04X\r\n",
         *((uint16*)(&MacAddr[6])),
         *((uint16*)(&MacAddr[4])),
         *((uint16*)(&MacAddr[2])),
         *((uint16*)(&MacAddr[0])));

  MacAddr = NLME_GetExtAddr();
  printf("Self_Nwk:%04X  ",NLME_GetShortAddr());
  printf("Self_Mac:%04X%04X%04X%04X\r\n",
         *((uint16*)(&MacAddr[6])),
         *((uint16*)(&MacAddr[4])),
         *((uint16*)(&MacAddr[2])),
         *((uint16*)(&MacAddr[0])));
}

afStatus_t user_send_data( afAddrMode_t mode, uint16 addr, char *data_buf )
{
  static uint8 transID;
  afAddrType_t dstAddr;
  afStatus_t status;

  dstAddr.addrMode = mode;
  dstAddr.endPoint = PROJECTAPP_ENDPOINT;
  dstAddr.addr.shortAddr = addr;

  status= AF_DataRequest( &dstAddr,
                          &ProjectApp_epDesc,
                          PROJECTAPP_CLUSTERID,
                          strlen((const char *)data_buf)+1,
                          (uint8 *)data_buf,
                          &transID,
                          AF_DISCV_ROUTE,
                          AF_DEFAULT_RADIUS );

  return status;
}
void user_state_change( devStates_t state )
{
  printf("DEV:%s\r\n",devStates_str[state]);

  if((state == DEV_ZB_COORD)
   ||(state == DEV_ROUTER)
   ||(state == DEV_END_DEVICE))
  {
    printf("\r\n");
    user_show_info();
    user_send_data((afAddrMode_t)Addr16Bit, 0x0000, "New Device Join");
  }
}
