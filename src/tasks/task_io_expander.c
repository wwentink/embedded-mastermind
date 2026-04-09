/**
 * @file io_expander.c
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2023-09-01
 *
 * @copyright Copyright (c) 2023
 *
 */
#include "main.h"
#if defined(ECE353_FREERTOS)
#include "cyhal_gpio.h"
#include "task_io_expander.h"
#include "task_console.h"
#include "rtos_events.h"
#include "devices.h"

/******************************************************************************/
/* Function Declarations                                                      */
/******************************************************************************/
static void task_io_expander(void *param);

/******************************************************************************/
/* Global Variables                                                           */
/******************************************************************************/
static cyhal_i2c_t *I2C_Obj;
static SemaphoreHandle_t *I2C_Semaphore = NULL;

/* Queue used to send commands used to io expander */
QueueHandle_t Queue_IO_Expander_Requests;

/******************************************************************************/
/* Static Function Definitions                                                */
/******************************************************************************/

/******************************************************************************/
/* Public Function Definitions                                                */
/******************************************************************************/

bool system_sensors_io_expander_write(QueueHandle_t return_queue, uint8_t address, uint8_t value)
{
	device_request_msg_t request_packet;
	device_response_msg_t response_packet;

	request_packet.device = DEVICE_IO_EXP;
	request_packet.operation = DEVICE_OP_WRITE;
	request_packet.address = address;
	request_packet.value = value;
	request_packet.response_queue = return_queue;
	xQueueSend(Queue_IO_Expander_Requests, &request_packet, portMAX_DELAY);

	if (return_queue != NULL)
	{
		xQueueReceive(return_queue, &response_packet, portMAX_DELAY);
	}

	return true;

}

bool system_sensors_io_expander_read(QueueHandle_t return_queue, uint8_t address, uint8_t *value)
{
	device_request_msg_t request_packet;
	device_response_msg_t response_packet;

	if(return_queue == NULL || value == NULL)
	{
		return false;
	}

	request_packet.device = DEVICE_IO_EXP;
	request_packet.operation = DEVICE_OP_READ;
	request_packet.address = address;
	request_packet.response_queue = return_queue;
	xQueueSend(Queue_IO_Expander_Requests, &request_packet, portMAX_DELAY);
	xQueueReceive(return_queue, &response_packet, portMAX_DELAY);
	*value = response_packet.payload.io_expander;


	return true;
}

/**
 * @brief
 * Task used to monitor the reception of command packets sent the io expander
 * @param param
 * Unused
 */
void task_io_expander(void *param)
{
	device_request_msg_t request_packet;
	device_response_msg_t response_packet;

	uint8_t read_value = 0;

	task_console_printf("Starting IO Expander Task\r\n");

	while (1)
	{
		/* Wait for a message */
		xQueueReceive(Queue_IO_Expander_Requests, &request_packet, portMAX_DELAY);

		if (request_packet.operation == DEVICE_OP_WRITE)
		{
			xSemaphoreTake(*I2C_Semaphore, portMAX_DELAY);
			i2c_write_u8(I2C_Obj, TCA9534_SUBORDINATE_ADDR, request_packet.address, request_packet.value);
			xSemaphoreGive(*I2C_Semaphore);

			response_packet.device = DEVICE_IO_EXP;
			response_packet.status = DEVICE_OPERATION_STATUS_WRITE_SUCCESS;
			if (request_packet.response_queue != NULL)
			{
				xQueueSend(request_packet.response_queue, &response_packet, portMAX_DELAY);
			}
		}
		else if (request_packet.operation == DEVICE_OP_READ)
		{
			xSemaphoreTake(*I2C_Semaphore, portMAX_DELAY);
			i2c_read_u8(I2C_Obj, TCA9534_SUBORDINATE_ADDR, request_packet.address, &read_value);
			xSemaphoreGive(*I2C_Semaphore);

			response_packet.device = DEVICE_IO_EXP;
			response_packet.status = DEVICE_OPERATION_STATUS_READ_SUCCESS;
			response_packet.payload.io_expander = read_value;
			xQueueSend(request_packet.response_queue, &response_packet, portMAX_DELAY);
		}
		else
		{
			task_console_printf("IO Expander received invalid request (device=%d, op=%d)\r\n", request_packet.device, request_packet.operation);
		}
	}
}

/**
 * @brief
 * Initializes software resources related to the operation of
 * the IO Expander.  This function expects that the I2C bus had already
 * been initialized prior to the start of FreeRTOS.
 */
bool task_io_expander_resources_init(SemaphoreHandle_t *i2c_semaphore, cyhal_i2c_t *i2c_obj)
{
	/* Save the I2C object and semaphore */
	I2C_Obj = i2c_obj;
	I2C_Semaphore = i2c_semaphore;
	if (I2C_Semaphore == NULL)
	{
		return false;
	}

	/* Create the Queue used to control blinking of the status LED*/
	Queue_IO_Expander_Requests = xQueueCreate(1, sizeof(device_request_msg_t));
	if (Queue_IO_Expander_Requests == NULL)
	{
		return false;
	}

	/* Create the task that will control the status LED */
	if(xTaskCreate(
		task_io_expander,
		"Task IO Exp",
		configMINIMAL_STACK_SIZE,
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