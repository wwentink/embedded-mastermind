/**
 * @file ice02.c
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2025-07-01
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include "main.h"

#if defined(ICE03)
#include "drivers.h"
#include <stdio.h>

char APP_DESCRIPTION[] = "ECE353: ICE 03 - Timer Interrupts/Debounce Buttons";

/*****************************************************************************/
/* Macros                                                                    */
/*****************************************************************************/

/*****************************************************************************/
/* Global Variables                                                          */
/*****************************************************************************/
typedef enum {
        INIT,
        SW1_DET,
        SW2_DET_1,
        SW2_DET_2,
        SW3_DET
    } current_state_t;

/*****************************************************************************/
/* Function Declarations                                                     */
/*****************************************************************************/

/*****************************************************************************/
/* Function Definitions                                                      */
/*****************************************************************************/

/**
 * @brief
 * This function will initialize all of the hardware resources for
 * the ICE
 */
void app_init_hw(void)
{
    cy_rslt_t rslt;

    console_init();
    printf("\x1b[2J\x1b[;H");
    printf("**************************************************\n\r");
    printf("* %s\n\r", APP_DESCRIPTION);
    printf("* Date: %s\n\r", __DATE__);
    printf("* Time: %s\n\r", __TIME__);
    printf("* Name:%s\n\r", NAME);
    printf("**************************************************\n\r");

    rslt = leds_init_gpio();
    if (rslt != CY_RSLT_SUCCESS)
    {
        printf("leds_init_gpio() failed: %d \n\r", rslt);
        CY_ASSERT(0);
    }

    rslt = buttons_init_gpio();
    if (rslt != CY_RSLT_SUCCESS)
    {
        printf("buttons_init_gpio() failed: %d \n\r", rslt);
        CY_ASSERT(0);
    }

    rslt = buttons_init_timer();
    if (rslt != CY_RSLT_SUCCESS)
    {
        printf("buttons_init_timer() failed: %d \n\r", rslt);
        CY_ASSERT(0);
    }

}

/*****************************************************************************/
/* Application Code                                                          */
/*****************************************************************************/
/**
 * @brief
 * This function implements the behavioral requirements for the ICE
 */
void app_main(void)
{   
    current_state_t current = INIT;

    while(1)
    {
        switch (current)
        {
            case INIT:
                leds_set_state(LED_GREEN, LED_STATE_OFF);
                leds_set_state(LED_BLUE, LED_STATE_OFF);
                leds_set_state(LED_RED, LED_STATE_ON);
                if (ECE353_Events.sw1)
                {
                    printf("SW1 Pressed\n\r");
                    ECE353_Events.sw1 = 0;
                    current = SW1_DET;
                } else if (ECE353_Events.sw2 || ECE353_Events.sw3) {
                    printf("Resetting to INIT\n\r");
                    ECE353_Events.sw2 = 0;
                    ECE353_Events.sw3 = 0;
                }
                break;
            case SW1_DET:
                leds_set_state(LED_BLUE, LED_STATE_ON);
                if (ECE353_Events.sw2)
                {
                    printf("SW2 Pressed\n\r");
                    ECE353_Events.sw2 = 0;
                    current = SW2_DET_1;
                } else if (ECE353_Events.sw1 || ECE353_Events.sw3) {
                    printf("Resetting to INIT\n\r");
                    ECE353_Events.sw1 = 0;
                    ECE353_Events.sw3 = 0;
                    current = INIT;
                }
                break;
            case SW2_DET_1:
                leds_set_state(LED_RED, LED_STATE_OFF);
                if (ECE353_Events.sw2)
                {
                    printf("SW2 Pressed Again\n\r");
                    ECE353_Events.sw2 = 0;
                    current = SW2_DET_2;
                } else if (ECE353_Events.sw1 || ECE353_Events.sw3) {
                    printf("Resetting to INIT\n\r");
                    ECE353_Events.sw1 = 0;
                    ECE353_Events.sw3 = 0;
                    current = INIT;
                }
                break;
            case SW2_DET_2:
                leds_set_state(LED_GREEN, LED_STATE_ON);
                if (ECE353_Events.sw3)
                {
                    printf("SW3 Pressed\n\r");
                    ECE353_Events.sw3 = 0;
                    current = SW3_DET;
                } else if (ECE353_Events.sw1 || ECE353_Events.sw2) {
                    printf("Resetting to INIT\n\r");
                    ECE353_Events.sw1 = 0;
                    ECE353_Events.sw2 = 0;
                    current = INIT;
                }
                break;
            case SW3_DET:
                leds_set_state(LED_BLUE, LED_STATE_OFF);
                if (ECE353_Events.sw1 || ECE353_Events.sw2 || ECE353_Events.sw3)
                {
                    printf("Resetting to INIT\n\r");
                    ECE353_Events.sw1 = 0;
                    ECE353_Events.sw2 = 0;
                    ECE353_Events.sw3 = 0;
                    current = INIT;
                }
                break;
            default:
                current = INIT;
                break;
        }
    }
}
#endif
