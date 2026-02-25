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

#if defined(EX08)
#include "drivers.h"
#include "rtos_events.h"
#include "task_buttons.h"
#include "task_lcd.h"
#include "task_joystick.h"

char APP_DESCRIPTION[] = "ECE353: Example 08 - FreeRTOS LCD Gatekeeper";

/*****************************************************************************/
/* Macros                                                                    */
/*****************************************************************************/

/*****************************************************************************/
/* Global Variables                                                          */
/*****************************************************************************/
EventGroupHandle_t ECE353_RTOS_Events;
QueueHandle_t xQueue_Request_LCD;

uint16_t sw1_presses = 0;
uint16_t sw2_presses = 0;
uint16_t sw3_presses = 0;

/*****************************************************************************/
/* Function Declarations                                                     */
/*****************************************************************************/

/*****************************************************************************/
/* Function Definitions                                                      */
/*****************************************************************************/
void task_system_control(void *pvParameters)
{
    (void)pvParameters; // Unused parameter

    EventBits_t events;
    
    lcd_msg_t lcd_msg;
    lcd_msg_request_t lcd_request;
    
    // Clear the screen
    lcd_msg.command = LCD_CMD_CLEAR_SCREEN;
    lcd_request.msg = lcd_msg;
    lcd_request.return_queue = NULL;
    xQueueSend(xQueue_Request_LCD, &lcd_request, portMAX_DELAY);

    while(1)
    {
        // Wait for any button events (SW1, SW2, or SW3)
        events = xEventGroupWaitBits(
            ECE353_RTOS_Events, 
            ECE353_EVENT_SW1_PRESSED | ECE353_EVENT_SW2_PRESSED | ECE353_EVENT_SW3_PRESSED,
            pdTRUE,  // Clear bits after wait
            pdFALSE, // Don't require all bits
            portMAX_DELAY
        );
        
        // Update counts based on which buttons were pressed
        if (events & ECE353_EVENT_SW1_PRESSED)
        {
            sw1_presses++;
        }
        if (events & ECE353_EVENT_SW2_PRESSED)
        {
            sw2_presses++;
        }
        if (events & ECE353_EVENT_SW3_PRESSED)
        {
            sw3_presses++;
        }

        // Clear the screen before printing new counts
        lcd_msg.command = LCD_CMD_CLEAR_SCREEN;
        lcd_request.msg = lcd_msg;
        lcd_request.return_queue = NULL;
        xQueueSend(xQueue_Request_LCD, &lcd_request, portMAX_DELAY);
        
        // Print the button press counts to the LCD
        lcd_msg.command = LCD_CMD_PRINT_MESSAGE;
        sprintf(lcd_msg.payload.message, "SW1:%u SW2:%u SW3:%u", sw1_presses, sw2_presses, sw3_presses);
        lcd_request.msg = lcd_msg;
        lcd_request.return_queue = NULL;
        xQueueSend(xQueue_Request_LCD, &lcd_request, portMAX_DELAY);
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

    rslt = buttons_init_gpio();
    if (rslt != CY_RSLT_SUCCESS)
    {
        printf("Buttons initialization failed!\n\r");
        for(int i = 0; i < 100000; i++) {}
        CY_ASSERT(0);
    }

    rslt = lcd_initialize();
    if (rslt != CY_RSLT_SUCCESS)
    {
        printf("LCD initialization failed!\n\r");
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

    /* Create the LCD Queue */
    xQueue_Request_LCD = xQueueCreate(10, sizeof(lcd_msg_request_t));
    if (xQueue_Request_LCD == NULL)
    {
        printf("Failed to create LCD queue\n\r");
        for(int i = 0; i < 100000; i++) {}
        CY_ASSERT(0);
    }

    /* Initialize the Button Task resources */
    if (!task_button_init())
    {
        printf("Failed to initialize button task\n\r");
        for(int i = 0; i < 100000; i++) {}
        CY_ASSERT(0); // If the task initialization fails, assert
    }

    /* Initialize LCD resources */
    if (!task_lcd_resources_init(xQueue_Request_LCD))
    {
        printf("Failed to initialize lcd task\n\r");
        for(int i = 0; i < 100000; i++) {}
       CY_ASSERT(0); // If the task initialization fails, assert
    }

    xTaskCreate(
        task_system_control, 
        "Task System Control", 
        configMINIMAL_STACK_SIZE*10, 
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