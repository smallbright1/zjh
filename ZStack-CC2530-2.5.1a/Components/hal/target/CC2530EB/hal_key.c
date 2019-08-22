#include "hal_mcu.h"
#include "hal_defs.h"
#include "hal_types.h"
#include "hal_board.h"
#include "hal_drivers.h"
#include "hal_adc.h"
#include "hal_key.h"
#include "osal.h"

#if (defined HAL_KEY) && (HAL_KEY == TRUE)

#define HAL_KEY_RISING_EDGE   0
#define HAL_KEY_FALLING_EDGE  1

#define HAL_KEY_DEBOUNCE_VALUE  25

/* Pull up or down register*/
#define HAL_KEY_UP_DOWN_PULL  P2INP

/* SW_1 is at P0_1 */
#define HAL_KEY_SW_1_BIT         BV(1)
#define HAL_KEY_SW_1_SEL         P0SEL
#define HAL_KEY_SW_1_DIR         P0DIR
#define HAL_KEY_SW_1_INP         P0INP
#define HAL_KEY_SW_1_INPBIT      BV(5)
/* edge interrupt */
#define HAL_KEY_SW_1_EDGE        HAL_KEY_FALLING_EDGE
#define HAL_KEY_SW_1_EDGEBIT     BV(0)
/* SW_1 interrupts */
#define HAL_KEY_SW_1_PORT_IEN    IEN1
#define HAL_KEY_SW_1_PORT_IENBIT BV(5)
#define HAL_KEY_SW_1_BIT_IEN     P0IEN
#define HAL_KEY_SW_1_PXIFG       P0IFG
#define HAL_KEY_SW_1_PXIF        P0IF

/* SW_2 is at P2_0 */
#define HAL_KEY_SW_2_BIT         BV(0)
#define HAL_KEY_SW_2_SEL         P2SEL
#define HAL_KEY_SW_2_DIR         P2DIR
#define HAL_KEY_SW_2_INP         P2INP
#define HAL_KEY_SW_2_INPBIT      BV(7)
/* edge interrupt */
#define HAL_KEY_SW_2_EDGE        HAL_KEY_RISING_EDGE
#define HAL_KEY_SW_2_EDGEBIT     BV(3)
/* SW_2 interrupts */
#define HAL_KEY_SW_2_PORT_IEN    IEN2
#define HAL_KEY_SW_2_PORT_IENBIT BV(1)
#define HAL_KEY_SW_2_BIT_IEN     P2IEN
#define HAL_KEY_SW_2_PXIFG       P2IFG
#define HAL_KEY_SW_2_PXIF        P2IF

static halKeyCBack_t pHalKeyProcessFunction;
static uint8 HalKeyConfigured;
bool Hal_KeyIntEnable;            /* interrupt enable/disable flag */

void halProcessKeyInterrupt(void);

/**************************************************************************************************
 * @fn      HalKeyInit
 *
 * @brief   Initilize Key Service
 *
 * @param   none
 *
 * @return  None
 **************************************************************************************************/
void HalKeyInit( void )
{
  HAL_KEY_SW_1_SEL     &= ~(HAL_KEY_SW_1_BIT);/* Set pin function to GPIO */
  HAL_KEY_SW_1_DIR     &= ~(HAL_KEY_SW_1_BIT);/* Set pin direction to Input */
  HAL_KEY_SW_1_INP     &= ~(HAL_KEY_SW_1_BIT);
  HAL_KEY_UP_DOWN_PULL &= ~(HAL_KEY_SW_1_INPBIT);/* UP PULL */

  HAL_KEY_SW_2_SEL     &= ~(HAL_KEY_SW_2_BIT);/* Set pin function to GPIO */
  HAL_KEY_SW_2_DIR     &= ~(HAL_KEY_SW_2_BIT);/* Set pin direction to Input */
  HAL_KEY_SW_2_INP     &= ~(HAL_KEY_SW_2_BIT);
  HAL_KEY_UP_DOWN_PULL |=   HAL_KEY_SW_2_INPBIT;/* DOWN PULL */

  /* Initialize callback function */
  pHalKeyProcessFunction  = NULL;

  /* Start with key is not configured */
  HalKeyConfigured = FALSE;
}
/**************************************************************************************************
 * @fn      HalKeyConfig
 *
 * @brief   Configure the Key serivce
 *
 * @param   interruptEnable - TRUE/FALSE, enable/disable interrupt
 *          cback - pointer to the CallBack function
 *
 * @return  None
 **************************************************************************************************/
void HalKeyConfig (bool interruptEnable, halKeyCBack_t cback)
{
  /* Enable/Disable Interrupt or */
  Hal_KeyIntEnable = interruptEnable;

  /* Register the callback fucntion */
  pHalKeyProcessFunction = cback;

  /* Determine if interrupt is enable or not */
  if (Hal_KeyIntEnable)
  {
    /* interrupt configuration for S1 */
    HAL_KEY_SW_1_PORT_IEN |= HAL_KEY_SW_1_PORT_IENBIT;
    HAL_KEY_SW_1_BIT_IEN  |= HAL_KEY_SW_1_BIT;
  #if (HAL_KEY_SW_1_EDGE == HAL_KEY_FALLING_EDGE)
    PICTL |= HAL_KEY_SW_1_EDGEBIT;
  #else
    PICTL &= ~(HAL_KEY_SW_1_EDGEBIT);
  #endif
    HAL_KEY_SW_1_PXIFG = 0x00;

    /* interrupt configuration for S2 */
    HAL_KEY_SW_2_PORT_IEN |= HAL_KEY_SW_2_PORT_IENBIT;
    HAL_KEY_SW_2_BIT_IEN  |= HAL_KEY_SW_2_BIT;
  #if (HAL_KEY_SW_2_EDGE == HAL_KEY_FALLING_EDGE)
    PICTL |= HAL_KEY_SW_2_EDGEBIT;
  #else
    PICTL &= ~(HAL_KEY_SW_2_EDGEBIT);
  #endif
    HAL_KEY_SW_2_PXIFG = 0x00;

    /* Do this only after the hal_key is configured - to work with sleep stuff */
    if (HalKeyConfigured == TRUE)
    {
      osal_stop_timerEx(Hal_TaskID, HAL_KEY_EVENT);  /* Cancel polling if active */
    }
  }
  else    /* Interrupts NOT enabled */
  {
    HAL_KEY_SW_1_BIT_IEN  &= ~(HAL_KEY_SW_1_BIT);        /* don't generate interrupt */
    HAL_KEY_SW_1_PORT_IEN &= ~(HAL_KEY_SW_1_PORT_IENBIT);/* Clear interrupt enable bit */

    HAL_KEY_SW_2_BIT_IEN  &= ~(HAL_KEY_SW_2_BIT);        /* don't generate interrupt */
    HAL_KEY_SW_2_PORT_IEN &= ~(HAL_KEY_SW_2_PORT_IENBIT);/* Clear interrupt enable bit */

    osal_set_event(Hal_TaskID, HAL_KEY_EVENT);
  }

  /* Key now is configured */
  HalKeyConfigured = TRUE;
}
/**************************************************************************************************
 * @fn      HalKeyRead
 *
 * @brief   Read the current value of a key
 *
 * @param   None
 *
 * @return  keys - current keys status
 **************************************************************************************************/
uint8 HalKeyRead ( void )
{
  uint8 keys = 0;

  if (HAL_PUSH_BUTTON1())
  {
    keys |= HAL_KEY_SW_1;
  }
  if (HAL_PUSH_BUTTON2())
  {
    keys |= HAL_KEY_SW_2;
  }

  return keys;
}
/**************************************************************************************************
 * @fn      HalKeyPoll
 *
 * @brief   Called by hal_driver to poll the keys
 *
 * @param   None
 *
 * @return  None
 **************************************************************************************************/
void HalKeyPoll (void)
{
  uint8 keys = 0;

  if (HAL_PUSH_BUTTON1())
  {
    keys |= HAL_KEY_SW_1;
  }
  if (HAL_PUSH_BUTTON2())
  {
    keys |= HAL_KEY_SW_2;
  }

  /* Invoke Callback if new keys were depressed */
  if (keys && (pHalKeyProcessFunction))
  {
    (pHalKeyProcessFunction) (keys, HAL_KEY_STATE_NORMAL);
  }
}
/**************************************************************************************************
 * @fn      halProcessKeyInterrupt
 *
 * @brief   Checks to see if it's a valid key interrupt, saves interrupt driven key states for
 *          processing by HalKeyRead(), and debounces keys by scheduling HalKeyRead() 25ms later.
 *
 * @param
 *
 * @return
 **************************************************************************************************/
void halProcessKeyInterrupt (void)
{
  bool valid=FALSE;

  if (HAL_KEY_SW_1_PXIFG & HAL_KEY_SW_1_BIT)  /* Interrupt Flag has been set */
  {
    HAL_KEY_SW_1_PXIFG = ~(HAL_KEY_SW_1_BIT); /* Clear Interrupt Flag */
    valid = TRUE;
  }
  if (HAL_KEY_SW_2_PXIFG & HAL_KEY_SW_2_BIT)  /* Interrupt Flag has been set */
  {
    HAL_KEY_SW_2_PXIFG = ~(HAL_KEY_SW_2_BIT); /* Clear Interrupt Flag */
    valid = TRUE;
  }

  if (valid)
  {
    osal_start_timerEx (Hal_TaskID, HAL_KEY_EVENT, HAL_KEY_DEBOUNCE_VALUE);
  }
}
/**************************************************************************************************
 * @fn      HalKeyEnterSleep
 *
 * @brief  - Get called to enter sleep mode
 *
 * @param
 *
 * @return
 **************************************************************************************************/
void HalKeyEnterSleep ( void )
{
}
/**************************************************************************************************
 * @fn      HalKeyExitSleep
 *
 * @brief   - Get called when sleep is over
 *
 * @param
 *
 * @return  - return saved keys
 **************************************************************************************************/
uint8 HalKeyExitSleep ( void )
{
  /* Wake up and read keys */
  return ( HalKeyRead () );
}
/***************************************************************************************************
 *                                    INTERRUPT SERVICE ROUTINE
 ***************************************************************************************************/

/**************************************************************************************************
 * @fn      halKeyPort0Isr
 *
 * @brief   Port0 ISR
 *
 * @param
 *
 * @return
 **************************************************************************************************/
HAL_ISR_FUNCTION( halKeyPort0Isr, P0INT_VECTOR )
{
  HAL_ENTER_ISR();

  if (HAL_KEY_SW_1_PXIFG & HAL_KEY_SW_1_BIT)
  {
    halProcessKeyInterrupt();
  }

  HAL_KEY_SW_1_PXIFG = 0;
  HAL_KEY_SW_1_PXIF = 0;
  
  CLEAR_SLEEP_MODE();
  HAL_EXIT_ISR();
}
/**************************************************************************************************
 * @fn      halKeyPort2Isr
 *
 * @brief   Port2 ISR
 *
 * @param
 *
 * @return
 **************************************************************************************************/
HAL_ISR_FUNCTION( halKeyPort2Isr, P2INT_VECTOR )
{
  HAL_ENTER_ISR();
  
  if (HAL_KEY_SW_2_PXIFG & HAL_KEY_SW_2_BIT)
  {
    halProcessKeyInterrupt();
  }

  HAL_KEY_SW_2_PXIFG = 0;
  HAL_KEY_SW_2_PXIF = 0;

  CLEAR_SLEEP_MODE();
  HAL_EXIT_ISR();
}
#else
void HalKeyInit(void){}
void HalKeyConfig(bool interruptEnable, halKeyCBack_t cback){}
uint8 HalKeyRead(void){ return 0;}
void HalKeyPoll(void){}
#endif /* HAL_KEY */
