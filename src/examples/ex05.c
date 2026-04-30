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

#if defined(EX05)

#include "drivers.h"

char APP_DESCRIPTION[] = "ECE353: Example 05 - FreeRTOS Tasks";

/*****************************************************************************/
/* Macros                                                                    */
/*****************************************************************************/

/*****************************************************************************/
/* Global Variables                                                          */
/*****************************************************************************/
volatile bool buzzer_enable = false;
/*****************************************************************************/
/* Function Declarations                                                     */
/*****************************************************************************/
void task_button_sw1(void *arg);

void task_button_sw2(void *arg);

void task_buzzer(void *arg);

/*****************************************************************************/
/* Function Definitions                                                      */
/*****************************************************************************/
void task_button_sw1(void *arg)
{
    (void)arg; // Unused parameter
    uint32_t button_count = 0;

    printf("Button SW1 Task Started!\n");
    while(1)
    {
        // check the button
        if ((PORT_BUTTON_SW1->IN & MASK_BUTTON_PIN_SW1) == 0)
        {
            button_count++;
            
            if (button_count == 2)
            {
                printf("Button SW1 Pressed!\n");
                buzzer_enable = true;
            }
        }
        else {
                button_count = 0;
        }

        // delay 15ms
        vTaskDelay(pdMS_TO_TICKS(15));
    }
}

void task_button_sw2(void *arg)
{
    (void)arg; // Unused parameter

    uint32_t button_count = 0;

    printf("Button SW2 Task Started!\n");
    while(1)
    {
        // check the button
        if ((PORT_BUTTON_SW2->IN & MASK_BUTTON_PIN_SW2) == 0)
        {
            button_count++;
            
            if (button_count == 2)
            {
                printf("Button SW2 Pressed!\n");
                buzzer_enable = false;
            }
        }
        else {
                button_count = 0;
        }

            // delay 15ms
            vTaskDelay(pdMS_TO_TICKS(15));
    }
}

void task_buzzer(void *arg)
{
    (void)arg; // Unused parameter

    printf("Buzzer Task Started!\n");
    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(100));

        if (buzzer_enable)
        {
            printf("Buzzer ON!\n");
            buzzer_on();
        }
        else
        {
            printf("Buzzer OFF!\n");
            buzzer_off();
        }
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

    /* Initialize the buttons */
    buttons_init_gpio();

    /* Initialize the buzzer */
    buzzer_init(100, 2000);
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
    xTaskCreate(
        task_button_sw1,            // Function used to implement a task
        "Button SW1 Task",          // Task name
        configMINIMAL_STACK_SIZE,   // Stack size
        NULL,                       // Not using any params so pass null
        tskIDLE_PRIORITY + 1,       // Task Priority
        NULL);                      // Task Handle
    xTaskCreate(task_button_sw2, 
        "Button SW2 Task",          
        configMINIMAL_STACK_SIZE, 
        NULL, 
        2, 
        NULL);
    xTaskCreate(task_buzzer, 
        "Buzzer Task", 
        configMINIMAL_STACK_SIZE, 
        NULL, 
        1, 
        NULL);

    /* Start the scheduler*/
    vTaskStartScheduler();

    /* Will never reach this loop once the scheduler starts */
    while (1)
    {
    }
}
#endif