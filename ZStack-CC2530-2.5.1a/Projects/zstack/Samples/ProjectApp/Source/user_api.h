#ifndef USER_API_H
#define USER_API_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "ZDApp.h"
#include "AF.h"

void user_show_info(void);
afStatus_t user_send_data( afAddrMode_t mode, uint16 addr, char *data_buf );
void user_state_change( devStates_t state );

#ifdef __cplusplus
}
#endif

#endif /* USER_API_H */
