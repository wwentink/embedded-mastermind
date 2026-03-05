/**
 * @file task_buttons.c
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2025-08-13
 * 
 * @copyright Copyright (c) 2025
 * 
 */

 #include "task_buttons.h"
 #include "task_console.h"

 #ifdef ECE353_FREERTOS
 /**
  * @brief 
  * Task used to debounce button presses (SW1, SW2, SW3).  
  * The falling edge of the button press is detected by de-bouncing
  * the button for 30mS. Each button should be sampled every 15mS.
  *
  * When a button press is detected, the corresponding event is set in
  * in the event group ECE353_RTOS_Events.
  *
  * @param arg 
  * Unused parameter
  */
 void task_buttons(void *arg)
 {
    (void)arg; // Unused parameter

    uint8_t sw1_prev = 1;
    uint8_t sw2_prev = 1;
    uint8_t sw3_prev = 1;

    while (1)
    {
        uint8_t sw1_now = cyhal_gpio_read(PIN_BUTTON_SW1);
        uint8_t sw2_now = cyhal_gpio_read(PIN_BUTTON_SW2);
        uint8_t sw3_now = cyhal_gpio_read(PIN_BUTTON_SW3);

        // Detect falling edge of SW1 with 30 mS debounce
        if ((sw1_prev == 1) && (sw1_now == 0))
        {
            vTaskDelay(pdMS_TO_TICKS(30)); // Debounce delay
            if (cyhal_gpio_read(PIN_BUTTON_SW1) == 0)
            {
                // Set event bit for SW1 pressed
                xEventGroupSetBits(ECE353_RTOS_Events, ECE353_EVENT_SW1_PRESSED);
                task_console_printf("SW1 Pressed\n");
            }
        }

        // Detect falling edge of SW2 with 30 mS debounce
        if ((sw2_prev == 1) && (sw2_now == 0))
        {
            vTaskDelay(pdMS_TO_TICKS(30)); // Debounce delay
            if (cyhal_gpio_read(PIN_BUTTON_SW2) == 0)
            {
                // Set event bit for SW2 pressed
                xEventGroupSetBits(ECE353_RTOS_Events, ECE353_EVENT_SW2_PRESSED);
                task_console_printf("SW2 Pressed\n");
            }
        }


        // Monitor button SW3
        if ((sw3_prev == 1) && (sw3_now == 0))
        {
            vTaskDelay(pdMS_TO_TICKS(30)); // Debounce delay
            if (cyhal_gpio_read(PIN_BUTTON_SW3) == 0)
            {
                // Set event bit for SW3 pressed
                xEventGroupSetBits(ECE353_RTOS_Events, ECE353_EVENT_SW3_PRESSED);
                task_console_printf("SW3 Pressed\n");
            }
        }

        sw1_prev = sw1_now;
        sw2_prev = sw2_now;
        sw3_prev = sw3_now;
  

        // Debounce delay
        vTaskDelay(pdMS_TO_TICKS(15));
    }
 }

 /* Button Task Initialization */
bool task_button_init(void){

    BaseType_t result;
    cy_rslt_t cy_result;

    // Initialize the button GPIO pins
    cy_result = buttons_init_gpio();
    if (cy_result != CY_RSLT_SUCCESS)
    {
        return false;
    }

    // Create the button task
    result = xTaskCreate(
        task_buttons, 
        "Button Task", 
        configMINIMAL_STACK_SIZE, 
        NULL, 
        tskIDLE_PRIORITY + 1, 
        NULL
    );

    if(result != pdPASS)
    {
        return false;
    }

    return true;
}
#endif