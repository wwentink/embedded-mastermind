/**
 * @file task_buzzer.c
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2025-08-13
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include "main.h"

#ifdef ECE353_FREERTOS

#include "task_buzzer.h"

/**
 * @brief 
 * Task used to control the buzzer based on button events.
 * 
 * SW1 -- Turn buzzer on
 * SW2 -- Turn buzzer off
 *
 * @param arg 
 * Unused parameter
 */
void task_buzzer(void *arg)
{
    (void)arg; // Unused parameter

    // Turn buzzer on (SW1) or off (SW2) based on button events

    while (1)
    {
        // Wait for either SW1 or SW2 to be pressed
        EventBits_t events = xEventGroupWaitBits(ECE353_RTOS_Events, 
                                                 ECE353_EVENT_SW1_PRESSED | ECE353_EVENT_SW2_PRESSED, 
                                                 pdTRUE, // Clear bits on exit
                                                 pdFALSE, // Wait for any bit
                                                 portMAX_DELAY); // Wait indefinitely

        if (events & ECE353_EVENT_SW1_PRESSED)
        {
            buzzer_on();
        }
        if (events & ECE353_EVENT_SW2_PRESSED)
        {
            buzzer_off();
        }
    }
}
#endif