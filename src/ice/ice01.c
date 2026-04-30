/**
 * @file ice01.c
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2025-07-01
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include "main.h"

#if defined(ICE01)
#include "drivers.h"

char APP_DESCRIPTION[] = "ECE353: ICE 01 - Memory Mapped IO - GPIO";

/*****************************************************************************/
/* Macros                                                                    */
/*****************************************************************************/

/*****************************************************************************/
/* Global Variables                                                          */
/*****************************************************************************/

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
    console_init();
    printf("\x1b[2J\x1b[;H");
    printf("**************************************************\n\r");
    printf("* %s\n\r", APP_DESCRIPTION);
    printf("* Date: %s\n\r", __DATE__);
    printf("* Time: %s\n\r", __TIME__);
    printf("* Name:%s\n\r", NAME);
    printf("**************************************************\n\r");

    /* Initialize the buttons */
    buttons_init_gpio();
    buttons_init_timer();
    printf("Buttons Initialized\n\r");

    /* Initialize the LEDs */
    leds_init_gpio();
    printf("LEDs Initialized\n\r");
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
    while(1)
    {
        /* Read the state of each button */
        button_state_t state_sw1 = buttons_get_state(BUTTON_SW1);
        button_state_t state_sw2 = buttons_get_state(BUTTON_SW2);
        button_state_t state_sw3 = buttons_get_state(BUTTON_SW3);

        /* SW1 - Red LED */
        if (state_sw1 == BUTTON_STATE_FALLING_EDGE) {
            printf("SW1 Falling Edge Detected\n\r");
            leds_set_state(LED_RED, LED_STATE_ON);
        }
        else if (state_sw1 == BUTTON_STATE_RISING_EDGE) {
            printf("SW1 Rising Edge Detected\n\r");
            leds_set_state(LED_RED, LED_STATE_OFF);
        }

        /* SW2 - Green LED */
        if (state_sw2 == BUTTON_STATE_FALLING_EDGE) {
            printf("SW2 Falling Edge Detected\n\r");
            leds_set_state(LED_GREEN, LED_STATE_ON);
        }
        else if (state_sw2 == BUTTON_STATE_RISING_EDGE) {
            printf("SW2 Rising Edge Detected\n\r");
            leds_set_state(LED_GREEN, LED_STATE_OFF);
        }

        /* SW3 - Blue LED */
        if (state_sw3 == BUTTON_STATE_FALLING_EDGE) {
            printf("SW3 Falling Edge Detected\n\r");
            leds_set_state(LED_BLUE, LED_STATE_ON);
        }
        else if (state_sw3 == BUTTON_STATE_RISING_EDGE) {
            printf("SW3 Rising Edge Detected\n\r");
            leds_set_state(LED_BLUE, LED_STATE_OFF);
        }

        /* Sleep for 100mS */
        cyhal_system_delay_ms(100);
    }
}
#endif
