/**
 * @file task_joystick.c
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2025-08-14
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include "main.h"

#ifdef ECE353_FREERTOS  
#include "drivers.h"
 #include "task_joystick.h"

 QueueHandle_t Queue_Joystick = NULL;

/* Message lookup table for joystick positions */
const char * const joystick_pos_names[] = {
    "Center",
    "Left",
    "Right",
    "Up",
    "Down",
    "Upper Left",
    "Upper Right",
    "Lower Left",
    "Lower Right"
};

 /**
  * @brief 
  *  Task used to monitor the joystick
  * @param arg 
  */
 void task_joystick(void *arg)
{
    (void)arg; // Unused parameter

    joystick_position_t position;
    joystick_position_t previous_position = JOYSTICK_POS_CENTER;

    while(1)
    {
        vTaskDelay(pdMS_TO_TICKS(500)); // Check joystick position every 500 ms

        position = joystick_get_pos();

        // Only add to queue if position has changed
        if(position != previous_position)
        {
            printf("Joystick Position: %s\n", joystick_pos_names[position]);
            
            // Send position to queue
            xQueueOverwrite(Queue_Joystick, &position);
            
            // Update previous position
            previous_position = position;
        }
    }
}


bool task_joystick_init(void)
{
    /* Create the Queue used to send Joystick Positions*/
    // Initialize queue to size 1 and hold joystick_position_t values
    Queue_Joystick = xQueueCreate(1, sizeof(joystick_position_t));

    /* Create the joystick task */
    // Register task_joystick with FreeRTOS
    xTaskCreate(task_joystick, "Joystick Task", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL);
        
    
    return true;
}
#endif