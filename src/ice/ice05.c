/**
 * @file ex03.c
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2025-06-30
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include "main.h"
#include "rtos_events.h"
#include "task_buttons.h"
#include "task_buzzer.h"

#if defined(ICE05)
#include "drivers.h"

char APP_DESCRIPTION[] = "ECE353: ICE 05 - FreeRTOS Event Groups";

/*****************************************************************************/
/* Macros                                                                    */
/*****************************************************************************/
// Event Group Bit Definitions (using definitions from rtos_events.h)

/*****************************************************************************/
/* Global Variables                                                          */
/*****************************************************************************/
// Event Group Handle
EventGroupHandle_t ECE353_RTOS_Events;

// Task Handles
static TaskHandle_t buzzer_task_handle;

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
    printf("**************************************************\n\r");
    printf("* %s\n\r", APP_DESCRIPTION);
    printf("* Date: %s\n\r", __DATE__);
    printf("* Time: %s\n\r", __TIME__);
    printf("* Name:%s\n\r", NAME);
    printf("**************************************************\n\r");

    /* Initialize the buttons */
    buttons_init_gpio();
    // Note: No timer-based interrupt needed - button monitoring is now handled by task_buttons()

    /* ADD CODE Initialize the buzzer */
    buzzer_init(50.0f, 1000); // Initialize buzzer with 50% duty cycle and 1kHz frequency
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
    /* Create the event group */
    ECE353_RTOS_Events = xEventGroupCreate();
    configASSERT(ECE353_RTOS_Events != NULL); // Ensure event group was created successfully
    
    /* Initialize the button task */
    task_button_init();
    
    /* Create and register the buzzer task with the RTOS scheduler */
    xTaskCreate(task_buzzer, "Buzzer Task", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, &buzzer_task_handle);
    configASSERT(buzzer_task_handle != NULL);

    /* Start the scheduler */
    vTaskStartScheduler();

    /* Will never reach this loop once the scheduler starts */
    while (1)
    {
    }
}
#endif