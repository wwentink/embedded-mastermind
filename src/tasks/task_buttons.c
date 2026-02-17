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

    while (1)
    {
        // Detect falling of SW1 with 30 mS debounce
        if (cyhal_gpio_read(PIN_BUTTON_SW1) == 0)
        {
            vTaskDelay(pdMS_TO_TICKS(30)); // Debounce delay
            if (cyhal_gpio_read(PIN_BUTTON_SW1) == 0)
            {
                // Set event bit for SW1 pressed
                xEventGroupSetBits(ECE353_RTOS_Events, ECE353_EVENT_SW1_PRESSED);
            }
        }
        

        // Monitor button SW2
        if (cyhal_gpio_read(PIN_BUTTON_SW2) == 0)
        {
            vTaskDelay(pdMS_TO_TICKS(30)); // Debounce delay
            if (cyhal_gpio_read(PIN_BUTTON_SW2) == 0)
            {
                // Set event bit for SW2 pressed
                xEventGroupSetBits(ECE353_RTOS_Events, ECE353_EVENT_SW2_PRESSED);
            }
        }


        // Monitor button SW3
        if (cyhal_gpio_read(PIN_BUTTON_SW3) == 0)
        {
            vTaskDelay(pdMS_TO_TICKS(30)); // Debounce delay
            if (cyhal_gpio_read(PIN_BUTTON_SW3) == 0)
            {
                // Set event bit for SW3 pressed
                xEventGroupSetBits(ECE353_RTOS_Events, ECE353_EVENT_SW3_PRESSED);
            }
        }
  

        // Debounce delay
    }
 }

 /* Button Task Initialization */
bool task_button_init(void){

    BaseType_t result;

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