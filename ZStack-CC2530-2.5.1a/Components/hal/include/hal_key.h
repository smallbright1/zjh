#ifndef HAL_KEY_H
#define HAL_KEY_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "hal_board.h"

/* Interrupt option - Enable or disable */
#define HAL_KEY_INTERRUPT_DISABLE    0x00
#define HAL_KEY_INTERRUPT_ENABLE     0x01

/* Key state - shift or nornal */
#define HAL_KEY_STATE_NORMAL         0x00
#define HAL_KEY_STATE_SHIFT          0x01

/* Switches (keys) */
#define HAL_KEY_SW_1 0x01  // Button 1£¬S1
#define HAL_KEY_SW_2 0x02  // Button 2£¬S2
#define HAL_KEY_SW_3 0x04  // Undefined
#define HAL_KEY_SW_4 0x08  // Undefined
#define HAL_KEY_SW_5 0x10  // Undefined
#define HAL_KEY_SW_6 0x20  // Undefined
#define HAL_KEY_SW_7 0x40  // Undefined

/**************************************************************************************************
 * TYPEDEFS
 **************************************************************************************************/
typedef void (*halKeyCBack_t) (uint8 keys, uint8 state);

/**************************************************************************************************
 *                                             GLOBAL VARIABLES
 **************************************************************************************************/
extern bool Hal_KeyIntEnable;

/**************************************************************************************************
 *                                             FUNCTIONS - API
 **************************************************************************************************/

/*
 * Initialize the Key Service
 */
extern void HalKeyInit( void );

/*
 * Configure the Key Service
 */
extern void HalKeyConfig( bool interruptEnable, const halKeyCBack_t cback);

/*
 * Read the Key status
 */
extern uint8 HalKeyRead( void);

/*
 * Enter sleep mode, store important values
 */
extern void HalKeyEnterSleep ( void );

/*
 * Exit sleep mode, retore values
 */
extern uint8 HalKeyExitSleep ( void );

/*
 * This is for internal used by hal_driver
 */
extern void HalKeyPoll ( void );

/*
 * This is for internal used by hal_sleep
 */
extern bool HalKeyPressed( void );

#ifdef __cplusplus
}
#endif

#endif
