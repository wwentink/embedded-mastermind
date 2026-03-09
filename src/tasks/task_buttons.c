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

    bool sw1_prev_pressed = false;
    bool sw2_prev_pressed = false;
    bool sw3_prev_pressed = false;

    while (1)
    {
        bool sw1_now_pressed = (cyhal_gpio_read(PIN_BUTTON_SW1) == 0);
        bool sw2_now_pressed = (cyhal_gpio_read(PIN_BUTTON_SW2) == 0);
        bool sw3_now_pressed = (cyhal_gpio_read(PIN_BUTTON_SW3) == 0);

        if (sw1_now_pressed && !sw1_prev_pressed)
        {
            vTaskDelay(pdMS_TO_TICKS(30));
            if (cyhal_gpio_read(PIN_BUTTON_SW1) == 0)
            {
                xEventGroupSetBits(ECE353_RTOS_Events, ECE353_EVENT_SW1_PRESSED);
            }
        }

        if (sw2_now_pressed && !sw2_prev_pressed)
        {
            vTaskDelay(pdMS_TO_TICKS(30));
            if (cyhal_gpio_read(PIN_BUTTON_SW2) == 0)
            {
                xEventGroupSetBits(ECE353_RTOS_Events, ECE353_EVENT_SW2_PRESSED);
            }
        }


        if (sw3_now_pressed && !sw3_prev_pressed)
        {
            vTaskDelay(pdMS_TO_TICKS(30));
            if (cyhal_gpio_read(PIN_BUTTON_SW3) == 0)
            {
                xEventGroupSetBits(ECE353_RTOS_Events, ECE353_EVENT_SW3_PRESSED);
            }
        }

        sw1_prev_pressed = sw1_now_pressed;
        sw2_prev_pressed = sw2_now_pressed;
        sw3_prev_pressed = sw3_now_pressed;
  

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