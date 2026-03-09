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
#include "devices.h"

/* External queue for joystick device requests */
extern QueueHandle_t Queue_Requests_Joystick;

/* External event group for system events */
extern EventGroupHandle_t ECE353_RTOS_Events;

/**
 * @brief 
 * Task used to monitor the joystick and publish events.
 * 
 * Periodically sends a DEVICE_OP_READ request to Queue_Requests_Joystick,
 * waits for response, and sets EventGroup bits based on joystick direction.
 * Also tracks return to CENTER position.
 * 
 * @param arg Unused parameter
 */
void task_joystick(void *arg)
{
    (void)arg; // Unused parameter

    device_request_msg_t request;
    device_response_msg_t response;
    joystick_position_t current_position = JOYSTICK_POS_CENTER;
    joystick_position_t previous_position = JOYSTICK_POS_CENTER;
    
    /* Create a local response queue for receiving joystick data */
    QueueHandle_t response_queue = xQueueCreate(1, sizeof(device_response_msg_t));
    if (response_queue == NULL)
    {
        printf("Failed to create joystick response queue\n\r");
        CY_ASSERT(0);
    }

    /* Initialize request structure */
    request.device = DEVICE_JOYSTICK;
    request.operation = DEVICE_OP_READ;
    request.address = 0;
    request.value = 0;
    request.response_queue = response_queue;

    /* Use vTaskDelayUntil for periodic polling */
    TickType_t last_wake_time = xTaskGetTickCount();
    const TickType_t poll_period = pdMS_TO_TICKS(50); // 50ms polling period

    while (1)
    {
        /* Send read request to joystick device */
        if (xQueueSend(Queue_Requests_Joystick, &request, pdMS_TO_TICKS(10)) == pdPASS)
        {
            /* Wait for response */
            if (xQueueReceive(response_queue, &response, pdMS_TO_TICKS(100)) == pdPASS)
            {
                if (response.status == DEVICE_OPERATION_STATUS_READ_SUCCESS)
                {
                    current_position = response.payload.joystick;
                    
                    /* Set EventGroup bits based on joystick position */
                    if (current_position != previous_position)
                    {
                        EventBits_t event_bits = 0;
                        
                        /* Determine which event bit to set based on joystick position */
                        event_bits = 0;
                        
                        if (current_position == JOYSTICK_POS_CENTER)
                        {
                            /* Joystick returned to center */
                            event_bits = EVENT_JOYSTICK_CENTER;
                        }
                        else if (current_position == JOYSTICK_POS_UP ||
                                 current_position == JOYSTICK_POS_UPPER_LEFT ||
                                 current_position == JOYSTICK_POS_UPPER_RIGHT)
                        {
                            event_bits = EVENT_JOYSTICK_UP;
                        }
                        else if (current_position == JOYSTICK_POS_DOWN ||
                                 current_position == JOYSTICK_POS_LOWER_LEFT ||
                                 current_position == JOYSTICK_POS_LOWER_RIGHT)
                        {
                            event_bits = EVENT_JOYSTICK_DOWN;
                        }
                        else if (current_position == JOYSTICK_POS_LEFT ||
                                 current_position == JOYSTICK_POS_UPPER_LEFT ||
                                 current_position == JOYSTICK_POS_LOWER_LEFT)
                        {
                            event_bits = EVENT_JOYSTICK_LEFT;
                        }
                        else if (current_position == JOYSTICK_POS_RIGHT ||
                                 current_position == JOYSTICK_POS_UPPER_RIGHT ||
                                 current_position == JOYSTICK_POS_LOWER_RIGHT)
                        {
                            event_bits = EVENT_JOYSTICK_RIGHT;
                        }
                        
                        /* Set the appropriate event bit(s) */
                        if (event_bits != 0)
                        {
                            xEventGroupSetBits(ECE353_RTOS_Events, event_bits);
                        }
                        
                        previous_position = current_position;
                    }
                }
            }
        }

        /* Wait until the next polling period */
        vTaskDelayUntil(&last_wake_time, poll_period);
    }
}


/**
 * @brief 
 * Initialize the joystick task and create it with FreeRTOS.
 * 
 * @return bool True if initialization was successful, false otherwise
 */
bool task_joystick_init(void)
{
    BaseType_t result;

    /* Create the joystick task */
    result = xTaskCreate(
        task_joystick,                      // Task function
        "Joystick Task",                    // Task name
        configMINIMAL_STACK_SIZE * 2,       // Stack size
        NULL,                               // Task parameters
        tskIDLE_PRIORITY + 1,               // Task priority
        NULL                                // Task handle
    );

    if (result != pdPASS)
    {
        printf("Failed to create joystick task\n\r");
        return false;
    }

    return true;
}

#endif