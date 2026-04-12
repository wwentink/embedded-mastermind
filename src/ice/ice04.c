/**
 * @file ice04.c
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2025-07-01
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include "main.h"

#if defined(ICE04)
#include "drivers.h"
#include <stdio.h>

char APP_DESCRIPTION[] = "ECE353: ICE 04 - PWM Buzzer";

/*****************************************************************************/
/* Macros                                                                    */
/*****************************************************************************/

/*****************************************************************************/
/* Global Variables                                                          */
/*****************************************************************************/
cyhal_pwm_t pwm_red;
cyhal_pwm_t pwm_green;
cyhal_pwm_t pwm_blue;

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
    
    /* Initialize the buttons */
    rslt = buttons_init_gpio();
    if (rslt != CY_RSLT_SUCCESS)
    {
        printf("Failed to initialize buttons GPIO\n");
        for(int i = 0; i < 1000000; i++); // Delay for a while
        CY_ASSERT(0);   
    }

    /* Initialize the timer for button debouncing */
    rslt = buttons_init_timer();
    if (rslt != CY_RSLT_SUCCESS)
    {
        printf("Error initializing button timer\n\r");
        for(int i = 0; i < 100000; i++);
        CY_ASSERT(0);
    }
    
    /* Initialize the RGB LED PWM */
    rslt = leds_init_pwm(&pwm_red, &pwm_green, &pwm_blue);
    if (rslt != CY_RSLT_SUCCESS)
    {
        printf("Failed to initialize LED PWM\n");
        CY_ASSERT(0);
    }

    rslt = cyhal_pwm_start(&pwm_red);
    if (rslt != CY_RSLT_SUCCESS)
    {
        printf("Failed to start RED PWM\n");
        CY_ASSERT(0);
    }

    rslt = cyhal_pwm_start(&pwm_green);
    if (rslt != CY_RSLT_SUCCESS)
    {
        printf("Failed to start GREEN PWM\n");
        CY_ASSERT(0);
    }

    rslt = cyhal_pwm_start(&pwm_blue);
    if (rslt != CY_RSLT_SUCCESS)
    {
        printf("Failed to start BLUE PWM\n");
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
    static float red_duty_cycle = 0.0f;
    static float green_duty_cycle = 0.0f;
    static float blue_duty_cycle = 0.0f;

    while(1)
    {

        // If SW1 increase Red LED duty cycle by 10%
        if (ECE353_Events.sw1) {
            ECE353_Events.sw1 = 0;

            red_duty_cycle += 10.0f;
            if (red_duty_cycle > 100.0f) {
                red_duty_cycle = 0.0f; // Wrap around to 0%
            }

            cyhal_pwm_set_duty_cycle(&pwm_red, red_duty_cycle, 100);
            printf("Red Duty Cycle: %.1f%%\n", red_duty_cycle);
        }

        // If SW2 increase Green LED duty cycle by 10%
        if (ECE353_Events.sw2) {
            ECE353_Events.sw2 = 0;
            
            green_duty_cycle += 10.0f;
            if (green_duty_cycle > 100.0f) {
                green_duty_cycle = 0.0f; // Wrap around to 0%
            }
            cyhal_pwm_set_duty_cycle(&pwm_green, green_duty_cycle, 100);
            printf("Green Duty Cycle: %.1f%%\n", green_duty_cycle);
        }

        // If SW3 increase Blue LED duty cycle by 10%
        if (ECE353_Events.sw3) {
            ECE353_Events.sw3 = 0;

            blue_duty_cycle += 10.0f;
            if (blue_duty_cycle > 100.0f) {
                blue_duty_cycle = 0.0f; // Wrap around to 0%
            }
            cyhal_pwm_set_duty_cycle(&pwm_blue, blue_duty_cycle, 100);
            printf("Blue Duty Cycle: %.1f%%\n", blue_duty_cycle);
        }
        /* END ADD CODE */

    }
}
#endif
