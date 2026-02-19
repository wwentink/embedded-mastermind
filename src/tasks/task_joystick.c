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

    uint16_t x_value, y_value;

    while(1)
    {
        vTaskDelay(pdMS_TO_TICKS(500)); // Delay to allow other tasks to initialize
        
        x_value = joystick_read_x();
        y_value = joystick_read_y();

        float x_voltage = (x_value / 65535.0) * 3.3; // Convert to voltage
        float y_voltage = (y_value / 65535.0) * 3.3; // Convert to voltage

        printf("Joystick X: %u (%.2f V), Y: %u (%.2f V)\n", x_value, x_voltage, y_value, y_voltage);
    }
}


bool task_joystick_init(void)
{
    /* Create the Queue used to send Joystick Positions*/

    /* Create the joystick task */
    
    return true;
}
#endif