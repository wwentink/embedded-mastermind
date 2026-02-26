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
EventGroupHandle_t ECE353_RTOS_Events;
uint16_t sw1_presses = 0;
uint16_t sw2_presses = 0;
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
        xEventGroupWaitBits(
            ECE353_RTOS_Events,
            ECE353_EVENT_SW1_PRESSED,
            pdTRUE,
            pdTRUE,
            portMAX_DELAY
        );

        // Wait 1 second to verify button is held
        vTaskDelay(pdMS_TO_TICKS(1000));
        
        // Check if button is still pressed
        if (cyhal_gpio_read(PIN_BUTTON_SW1) == 0)
        {
            sw1_presses++;

            lcd_msg_t msg;
            lcd_msg_request_t lcd_request;
            msg.command = LCD_CMD_PRINT_SW1_COUNT;
            snprintf(msg.payload.message, sizeof(msg.payload.message), "SW1: %d", sw1_presses);

            lcd_request.msg = msg;
            lcd_request.return_queue = NULL;

            xQueueSend(Queue_LCD_Request, &lcd_request, portMAX_DELAY);
        }
    }
}

void task_sw2(void *pvParameters)
{
    (void)pvParameters; // Unused parameter

    printf("Starting Task SW2\n\r");
    while(1)
    {
        xEventGroupWaitBits(
            ECE353_RTOS_Events,
            ECE353_EVENT_SW2_PRESSED,
            pdTRUE,
            pdTRUE,
            portMAX_DELAY
        );

        // Wait 1 second to verify button is held
        vTaskDelay(pdMS_TO_TICKS(1000));
        
        // Check if button is still pressed
        if (cyhal_gpio_read(PIN_BUTTON_SW2) == 0)
        {
            sw2_presses++;

            lcd_msg_t msg;
            lcd_msg_request_t lcd_request;
            msg.command = LCD_CMD_PRINT_SW2_COUNT;
            snprintf(msg.payload.message, sizeof(msg.payload.message), "SW2: %d", sw2_presses);

            lcd_request.msg = msg;
            lcd_request.return_queue = NULL;

            xQueueSend(Queue_LCD_Request, &lcd_request, portMAX_DELAY);
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
    Queue_LCD_Request = xQueueCreate(10, sizeof(lcd_msg_request_t));

    /* Initialize the LCD task */
    task_lcd_resources_init(Queue_LCD_Request);

    if (!task_button_init())
    {
        printf("Failed to initialize button task\n\r");
        for(int i = 0; i < 100000; i++) {}
        CY_ASSERT(0);
    }

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
