/**
 * @file task_device_gatekeeper.c
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2026-03-08
 * 
 * @copyright Copyright (c) 2026
 * 
 */

#include "main.h"

#if defined(ECE353_FREERTOS)
#include "drivers.h"
#include "devices.h"

extern QueueHandle_t Queue_Requests_Joystick;

/**
 * @brief
 * Device Gatekeeper Task - handles all device read/write requests
 * Currently handles joystick device requests
 * 
 * @param pvParameters Unused parameter
 */
void task_device_gatekeeper(void *pvParameters)
{
    (void)pvParameters; // Unused parameter
    
    device_request_msg_t request;
    device_response_msg_t response;

    while (1)
    {
        /* Block waiting for device request */
        if (xQueueReceive(Queue_Requests_Joystick, &request, portMAX_DELAY) == pdPASS)
        {
            /* Initialize response */
            response.device = request.device;
            response.status = DEVICE_OPERATION_STATUS_READ_FAILURE;
            
            /* Handle joystick read requests */
            if (request.device == DEVICE_JOYSTICK && request.operation == DEVICE_OP_READ)
            {
                /* Read joystick position */
                joystick_position_t pos = joystick_get_pos();
                response.payload.joystick = pos;
                response.status = DEVICE_OPERATION_STATUS_READ_SUCCESS;
            }
            
            /* Send response back to requester */
            if (request.response_queue != NULL)
            {
                xQueueSend(request.response_queue, &response, pdMS_TO_TICKS(10));
            }
        }
    }
}

/**
 * @brief
 * Initialize the device gatekeeper task
 * 
 * @return bool True if successful, false otherwise
 */
bool task_device_gatekeeper_init(void)
{
    BaseType_t result;

    /* Create the device gatekeeper task */
    result = xTaskCreate(
        task_device_gatekeeper,
        "Device Gatekeeper",
        configMINIMAL_STACK_SIZE * 2,
        NULL,
        tskIDLE_PRIORITY + 2,  /* Higher priority than joystick task */
        NULL
    );

    if (result != pdPASS)
    {
        printf("Failed to create device gatekeeper task\n\r");
        return false;
    }

    return true;
}

#endif
