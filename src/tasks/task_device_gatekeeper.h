/**
 * @file task_device_gatekeeper.h
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2026-03-08
 * 
 * @copyright Copyright (c) 2026
 * 
 */

#ifndef __TASK_DEVICE_GATEKEEPER_H__
#define __TASK_DEVICE_GATEKEEPER_H__

#include "main.h"

#ifdef ECE353_FREERTOS

/**
 * @brief
 * Device Gatekeeper Task - handles all device read/write requests
 * 
 * @param pvParameters Unused parameter
 */
void task_device_gatekeeper(void *pvParameters);

/**
 * @brief
 * Initialize the device gatekeeper task
 * 
 * @return bool True if successful, false otherwise
 */
bool task_device_gatekeeper_init(void);

#endif

#endif /* __TASK_DEVICE_GATEKEEPER_H__ */
