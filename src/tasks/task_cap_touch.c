/**
 * @file task_cap_touch.c
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2026-01-07
 * 
 * @copyright Copyright (c) 2026
 * 
 */
 #include "task_cap_touch.h"

#if defined(ECE353_FREERTOS)
#include "rtos_events.h"

/*****************************************************************************/
/* Global Variables                                                          */
/*****************************************************************************/
QueueHandle_t               Queue_Request_Cap_Touch = NULL;
static SemaphoreHandle_t    I2C_Semaphore = NULL;
static cyhal_i2c_t         *I2C_Obj = NULL;
static cyhal_gpio_t        Cap_Touch_Int_Pin = NC;

void task_cap_touch(void *param)
{
    (void)param; // Unused parameter
    device_request_msg_t request_packet;
    device_response_msg_t response_packet;
    uint8_t num_points = 0;
    uint16_t x_pos = 0;
    uint16_t y_pos = 0;

    // task_console_printf() prepends the task name automatically.
    task_console_printf("Starting Cap Touch Task\r\n");

    while(1)
    {
        /* Wait for a request to arrive */
        xQueueReceive(Queue_Request_Cap_Touch, &request_packet, portMAX_DELAY);

        response_packet.device = DEVICE_CAP_TOUCH;
        response_packet.status = DEVICE_OPERATION_STATUS_READ_FAILURE;
        response_packet.payload.cap_touch[0] = 0;
        response_packet.payload.cap_touch[1] = 0;

        if((request_packet.device == DEVICE_CAP_TOUCH) && (request_packet.operation == DEVICE_OP_READ))
        {
            xSemaphoreTake(I2C_Semaphore, portMAX_DELAY);

            if(cap_touch_get_num_points(I2C_Obj, &num_points) && (num_points > 0))
            {
                if(cap_touch_get_xy_position(I2C_Obj, &x_pos, &y_pos))
                {
                    response_packet.status = DEVICE_OPERATION_STATUS_READ_SUCCESS;
                    response_packet.payload.cap_touch[0] = x_pos;
                    response_packet.payload.cap_touch[1] = y_pos;
                }
            }

            xSemaphoreGive(I2C_Semaphore);
        }

        if(request_packet.response_queue != NULL)
        {
            xQueueSend(request_packet.response_queue, &response_packet, portMAX_DELAY);
        }
    }
}

bool task_cap_touch_resources_init(
    QueueHandle_t queue_request, 
    SemaphoreHandle_t i2c_semaphore, 
    cyhal_i2c_t *i2c_obj, 
    cyhal_gpio_t pin_cap_touch_int
)
{
    (void)queue_request;

    if(i2c_semaphore == NULL || i2c_obj == NULL)
    {
        return false;
    }   

    /* Create the queue used by the Cap Touch gatekeeper task. */
    Queue_Request_Cap_Touch = xQueueCreate(1, sizeof(device_request_msg_t));
    if(Queue_Request_Cap_Touch == NULL)
    {
        return false;
    }

    /* Save the resources */
    I2C_Semaphore = i2c_semaphore;
    I2C_Obj = i2c_obj;
    Cap_Touch_Int_Pin = pin_cap_touch_int;

    /* Create the cap touch task */
    if (xTaskCreate(
            task_cap_touch,
            // Match expected console prefix width/content for startup logs.
            "Cap Touch Task",
            TASK_CAP_TOUCH_STACK_SIZE,
            NULL,
            TASK_CAP_TOUCH_PRIORITY,
            NULL) != pdPASS)
    {
        return false;
    }

    return true;
}
#endif /* ECE353_FREERTOS */