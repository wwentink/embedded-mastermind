/**
 * @file ice08.c
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2025-06-30
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include "main.h"

#if defined(ICE08)
#include "drivers.h"        
#include "rtos_events.h"
#include "task_buttons.h"
#include "task_lcd.h"
#include "task_joystick.h"

char APP_DESCRIPTION[] = "ECE353: ICE 08 - FreeRTOS LCD Gatekeeper";

/*****************************************************************************/
/* Macros                                                                    */
/*****************************************************************************/

/*****************************************************************************/
/* Global Variables                                                          */
/*****************************************************************************/
/* ADD CODE */
/* FreeRTOS Queue for LCD messages */
QueueHandle_t Queue_LCD_Request = NULL;

/*****************************************************************************/
/* Function Declarations                                                     */
/*****************************************************************************/

/*****************************************************************************/
/* Function Definitions                                                      */
/*****************************************************************************/
void task_sw1(void *pvParameters)
{
    (void)pvParameters; // Unused parameter

    printf("Starting Task SW1\n\r");
    while(1)
    {
        // Sleep for 25 ms
        vTaskDelay(pdMS_TO_TICKS(25));

        /* ADD CODE */
    }
}

void task_sw2(void *pvParameters)
{
    (void)pvParameters; // Unused parameter

    printf("Starting Task SW2\n\r");
    while(1)
    {
        // Sleep for 25 ms
        vTaskDelay(pdMS_TO_TICKS(25));

        /* ADD CODE */
    }
}

/**
 * @brief
 * This function will initialize all of the hardware resources for
 * the ICE
 */
void app_init_hw(void)
{
    cy_rslt_t rslt;

    console_init();
    printf("**************************************************\n\r");
    printf("* %s\n\r", APP_DESCRIPTION);
    printf("* Date: %s\n\r", __DATE__);
    printf("* Time: %s\n\r", __TIME__);
    printf("* Name:%s\n\r", NAME);
    printf("**************************************************\n\r");

    rslt = lcd_initialize();
    if (rslt != CY_RSLT_SUCCESS)
    {
        printf("LCD initialization failed!\n\r");
        for(int i = 0; i < 100000; i++) {}
        CY_ASSERT(0);
    }

    rslt = buttons_init_gpio();
    if (rslt != CY_RSLT_SUCCESS)
    {
        printf("Buttons initialization failed!\n\r");
        for(int i = 0; i < 100000; i++) {}
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
    
    /* Register the tasks with FreeRTOS*/

    ECE353_RTOS_Events = xEventGroupCreate();

    /* Create the LCD Request Queue*/
    /* ADD CODE */

    /* Initialize the LCD task */
    /* ADD CODE */

    xTaskCreate(
        task_sw1, 
        "Task SW1", 
        configMINIMAL_STACK_SIZE*2, 
        NULL, 
        tskIDLE_PRIORITY + 1, 
        NULL
    );

    xTaskCreate(
        task_sw2, 
        "Task SW2", 
        configMINIMAL_STACK_SIZE*2, 
        NULL, 
        tskIDLE_PRIORITY + 1, 
        NULL
    );

    /* Start the scheduler*/
    vTaskStartScheduler();

    /* Will never reach this loop once the scheduler starts */
    while (1)
    {
    }
}
#endif
