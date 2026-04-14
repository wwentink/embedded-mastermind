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

/* Rotate raw touch readings into the LCD coordinate frame. */
#define CAP_TOUCH_RAW_X_MAX   239U
#define CAP_TOUCH_RAW_Y_MAX   319U

static uint16_t cap_touch_scale_to_range(uint16_t value, uint16_t in_max, uint16_t out_max)
{
    if(value >= in_max)
    {
        return out_max;
    }

    return (uint16_t)(((uint32_t)value * out_max) / in_max);
}

static void cap_touch_rotate_ccw(uint16_t raw_x, uint16_t raw_y, uint16_t *screen_x, uint16_t *screen_y)
{
    uint16_t scaled_x = cap_touch_scale_to_range(raw_y, CAP_TOUCH_RAW_Y_MAX, LCD_ROWS - 1U);
    uint16_t scaled_y = cap_touch_scale_to_range(raw_x, CAP_TOUCH_RAW_X_MAX, LCD_COLS - 1U);

    if(screen_x != NULL)
    {
        *screen_x = scaled_x;
    }

    if(screen_y != NULL)
    {
        *screen_y = (LCD_COLS - 1U) - scaled_y;
    }
}

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
                    uint16_t screen_x = 0;
                    uint16_t screen_y = 0;

                    cap_touch_rotate_ccw(x_pos, y_pos, &screen_x, &screen_y);

                    response_packet.status = DEVICE_OPERATION_STATUS_READ_SUCCESS;
                    response_packet.payload.cap_touch[0] = screen_x;
                    response_packet.payload.cap_touch[1] = screen_y;
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