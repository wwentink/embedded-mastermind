/**
 * @file devices.c
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2025-10-27
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include "devices.h"

#if defined(ECE353_FREERTOS)
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "task_imu.h"
#include "task_temp_sensor.h"
#include "task_light_sensor.h"
#include "task_io_expander.h"
#include "task_eeprom.h"
#include "task_console.h"

/**
 * @brief
 * Parse a CLI command string into a device request packet.
 *
 * Supported commands:
 *   EEPROM R <ADDR>
 *   EEPROM W <ADDR> <VAL>
 *   CAP_TOUCH
 *
 * @param data
 * Pointer to the command string.
 * @param request
 * Pointer to the request packet to populate.
 * @return true
 * Command parsed successfully.
 * @return false
 * Command is invalid or unsupported.
 */
bool parse_cli_data(char *data, device_request_msg_t *request)
{
	char device_name[16];
	char operation[8];
	char *cmd_ptr = data;
	unsigned int address = 0;
	unsigned int value = 0;
	int fields = 0;
	uint32_t i = 0;

	if ((data == NULL) || (request == NULL))
	{
		return false;
	}

	/* Clear the request packet before parsing the command fields. */
	memset(request, 0, sizeof(device_request_msg_t));
	request->device = DEVICE_UNKNOWN;

	/* Ignore leading whitespace before parsing command tokens. */
	while((*cmd_ptr != '\0') && isspace((unsigned char)*cmd_ptr))
	{
		cmd_ptr++;
	}

	/* Parse command fields; %i accepts both decimal and hexadecimal text. */
	fields = sscanf(cmd_ptr, "%15s %7s %i %i", device_name, operation, &address, &value);

	if(fields <= 0)
	{
		return false;
	}

	/* Normalize command tokens to uppercase to accept mixed-case input. */
	for(i = 0; i < strlen(device_name); i++)
	{
		device_name[i] = (char)toupper((unsigned char)device_name[i]);
	}

	for(i = 0; i < strlen(operation); i++)
	{
		operation[i] = (char)toupper((unsigned char)operation[i]);
	}

	if ((fields == 1) && (strcmp(device_name, "CAP_TOUCH") == 0))
	{
		request->device = DEVICE_CAP_TOUCH;
		request->operation = DEVICE_OP_READ;
		return true;
	}

	if ((fields >= 3) && (strcmp(device_name, "EEPROM") == 0))
	{
		if ((strcmp(operation, "R") == 0) && (fields == 3))
		{
			request->device = DEVICE_EEPROM;
			request->operation = DEVICE_OP_READ;
			request->address = (uint16_t)address;
			return true;
		}

		if ((strcmp(operation, "W") == 0) && (fields == 4))
		{
			request->device = DEVICE_EEPROM;
			request->operation = DEVICE_OP_WRITE;
			request->address = (uint16_t)address;
			request->value = (uint8_t)value;
			return true;
		}
	}

	return false;
}

#endif /* ECE353_FREERTOS */
