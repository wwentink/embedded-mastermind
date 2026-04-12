/**
 * @file task_light_sensor.c
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2025-09-18
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include "main.h"

#if defined(ECE353_FREERTOS)
#include "drivers.h"
#include "task_light_sensor.h"
#include "task_console.h"

/******************************************************************************/
/* Function Declarations                                                      */
/******************************************************************************/

/******************************************************************************/
/* Global Variables                                                           */
/******************************************************************************/
/* I2C Object Handle */
static cyhal_i2c_t *I2C_Obj;

/* I2C Semaphore */
static SemaphoreHandle_t *I2C_Semaphore = NULL;

/* Queue used to send commands used to light sensor */
QueueHandle_t Queue_Light_Sensor_Requests;


/******************************************************************************/
/* Static Function Definitions                                                */
/******************************************************************************/
/**
 * @brief
 * Set ALS MODE to Active and initiate a software reset
 */
static void ltr_light_sensor_start(void)
{
    cy_rslt_t rslt;

    /* Initiate software reset */
    rslt = i2c_write_u8(I2C_Obj, LTR_SUBORDINATE_ADDR, LTR_REG_CONTR, LTR_REG_CONTR_SW_RESET);
    if (rslt != CY_RSLT_SUCCESS)
    {
        printf("Failed to reset light sensor with error code: %lu\n", (unsigned long)rslt);
    }
    
    /* Set ALS measurement rate */
    rslt = i2c_write_u8(I2C_Obj, LTR_SUBORDINATE_ADDR, LTR_REG_MEAS_RATE, 0x03);
    if (rslt != CY_RSLT_SUCCESS)
    {
        printf("Failed to set ALS measurement rate with error code: %lu\n", (unsigned long)rslt);
    }

    /* Put ALS into active mode */
    rslt = i2c_write_u8(I2C_Obj, LTR_SUBORDINATE_ADDR, LTR_REG_CONTR, LTR_REG_CONTR_ALS_MODE);
    if (rslt != CY_RSLT_SUCCESS)
    {
        printf("Failed to enable ALS mode with error code: %lu\n", (unsigned long)rslt);
    }
}

static uint8_t ltr_light_sensor_status(void)
{
    uint8_t value = 0;
    cy_rslt_t rslt;

    rslt = i2c_read_u8(I2C_Obj, LTR_SUBORDINATE_ADDR, LTR_REG_ALS_STATUS, &value);
    if (rslt != CY_RSLT_SUCCESS)
    {
        printf("Failed to read light sensor status with error code: %lu\n", (unsigned long)rslt);
    }


    return value;
}

static uint8_t ltr_light_sensor_manufac_id(void)
{
    uint8_t value = 0;

    cy_rslt_t rslt;
    rslt = i2c_read_u8(I2C_Obj, LTR_SUBORDINATE_ADDR, LTR_REG_MANUFAC_ID, &value);
    if (rslt != CY_RSLT_SUCCESS)
    {
        printf("Failed to read light sensor manufacturer ID with error code: %lu\n", (unsigned long)rslt);
    }

    return value;
}

static uint16_t ltr_light_sensor_get_ch0(void)
{
    uint8_t msbyte;
    uint8_t lsbyte;

    cy_rslt_t rslt;
    rslt = i2c_read_u8(I2C_Obj, LTR_SUBORDINATE_ADDR, LTR_REG_ALS_DATA_CH0_0, &lsbyte);
    if (rslt != CY_RSLT_SUCCESS)
    {
        printf("Failed to read light sensor CH0 LSB with error code: %lu\n", (unsigned long)rslt);
    }

    rslt = i2c_read_u8(I2C_Obj, LTR_SUBORDINATE_ADDR, LTR_REG_ALS_DATA_CH0_1, &msbyte);
    if (rslt != CY_RSLT_SUCCESS)
    {
        printf("Failed to read light sensor CH0 MSB with error code: %lu\n", (unsigned long)rslt);
    }

    return (uint16_t)(msbyte << 8) | lsbyte;
}

static uint16_t ltr_light_sensor_get_ch1(void)
{
    uint8_t msbyte;
    uint8_t lsbyte;

    cy_rslt_t rslt;
    rslt = i2c_read_u8(I2C_Obj, LTR_SUBORDINATE_ADDR, LTR_REG_ALS_DATA_CH1_0, &lsbyte);
    if (rslt != CY_RSLT_SUCCESS)
    {
        printf("Failed to read light sensor CH1 LSB with error code: %lu\n", (unsigned long)rslt);
    }

    rslt = i2c_read_u8(I2C_Obj, LTR_SUBORDINATE_ADDR, LTR_REG_ALS_DATA_CH1_1, &msbyte);
    if (rslt != CY_RSLT_SUCCESS)
    {
        printf("Failed to read light sensor CH1 MSB with error code: %lu\n", (unsigned long)rslt);
    }

    return (uint16_t)(msbyte << 8) | lsbyte;
}

static void ltr_light_sensor_get_readings(uint16_t *ch1, uint16_t *ch0)
{
    uint8_t status = 0;

    status = ltr_light_sensor_status();
    while(((status & LTR_REG_STATUS_NEW_DATA) == 0) && ((status & LTR_REG_STATUS_VALID_DATA) == 0))
    {
        vTaskDelay(pdMS_TO_TICKS(1));
        status = ltr_light_sensor_status();
    }

    *ch1 = ltr_light_sensor_get_ch1();
    *ch0 = ltr_light_sensor_get_ch0();
}

/******************************************************************************/
/* Public Function Definitions                                                */
/******************************************************************************/
/*
* @brief 
 * This is a helper function that is called by tasks other than the light sensor task
 * when they want to read the ambient light from the sensor.
*/
bool system_sensors_get_light(QueueHandle_t return_queue, uint16_t *ambient_light)
{
    device_request_msg_t request_packet;
    device_response_msg_t response_packet;

    if(return_queue == NULL || ambient_light == NULL)
    {
        return false;
    }

    request_packet.device = DEVICE_LIGHT;
    request_packet.operation = DEVICE_OP_READ;
    request_packet.response_queue = return_queue;

    xQueueSend(Queue_Light_Sensor_Requests, &request_packet, portMAX_DELAY);
    xQueueReceive(return_queue, &response_packet, portMAX_DELAY);

    *ambient_light = response_packet.payload.light_sensor;
    if (response_packet.status == DEVICE_OPERATION_STATUS_READ_SUCCESS)
    {
        return true;
    }
    else
    {
        return false;
    }

    return true;
}


/**
 * @brief
 * Task used to monitor the reception of command packets sent the light sensor
 * @param param
 * Unused
 */
void task_light_sensor(void *param)
{
    device_request_msg_t request_packet;
    device_response_msg_t response_packet;
    uint8_t manufacturer_id = 0;

	task_console_printf("Starting Light Sensor Task\r\n");

    /* Verify that the device was found on the I2C bus */
    xSemaphoreTake(*I2C_Semaphore, portMAX_DELAY);
    manufacturer_id = ltr_light_sensor_manufac_id();
    xSemaphoreGive(*I2C_Semaphore);

    if (manufacturer_id != 0x05)
    {
        task_console_printf("Error: Incorrect manufacturer ID read from light sensor: 0x%02X\r\n", manufacturer_id);
        vTaskSuspend(NULL);
    }
    else
    {
        task_console_printf("Light sensor found! Manufacturer ID: 0x%02X\r\n", manufacturer_id);
    }

    /* Start the Light Sensor */
    xSemaphoreTake(*I2C_Semaphore, portMAX_DELAY);
    ltr_light_sensor_start();
    xSemaphoreGive(*I2C_Semaphore);

	while (1)
	{
		/* Wait for a message */
		xQueueReceive(Queue_Light_Sensor_Requests, &request_packet, portMAX_DELAY);

        if ((request_packet.device == DEVICE_LIGHT) && (request_packet.operation == DEVICE_OP_READ))
        {
            uint16_t ch0 = 0;
            uint16_t ch1 = 0;

            xSemaphoreTake(*I2C_Semaphore, portMAX_DELAY);
            ltr_light_sensor_get_readings(&ch1, &ch0);
            xSemaphoreGive(*I2C_Semaphore);

            response_packet.device = DEVICE_LIGHT;
            response_packet.status = DEVICE_OPERATION_STATUS_READ_SUCCESS;
            response_packet.payload.light_sensor = ch0;

            xQueueSend(request_packet.response_queue, &response_packet, portMAX_DELAY);
        }
        else
        {
            task_console_printf("Light sensor task received invalid request (device=%d, op=%d)\r\n",
                request_packet.device,
                request_packet.operation);
        }
	}
}

/**
 * @brief
 * Initializes software resources related to the operation of
 * the Temp Sensor.  This function expects that the I2C bus had already
 * been initialized prior to the start of FreeRTOS.
 */
bool task_light_sensor_resources_init(SemaphoreHandle_t *i2c_semaphore, cyhal_i2c_t *i2c_obj)
{
    /* Save the I2C Object */
    I2C_Obj = i2c_obj;

    /* Save the I2C Semaphore */
    I2C_Semaphore = i2c_semaphore;
    if (I2C_Semaphore == NULL || I2C_Obj == NULL)
    {
        return false;
    }

	/* Create the Queue used to receive requests from other tasks */
	Queue_Light_Sensor_Requests = xQueueCreate(1, sizeof(device_request_msg_t));
	if (Queue_Light_Sensor_Requests == NULL)
	{
		return false;
	}
	
	/* Create the task that will control the status LED */
	if(xTaskCreate(
		task_light_sensor,
		"Light Sensor",
        10*configMINIMAL_STACK_SIZE,
        i2c_semaphore,
		tskIDLE_PRIORITY + 1,
		NULL) != pdPASS)
	{
		return false;
	}
	else 
	{
		return true;
	}	
}	
#endif