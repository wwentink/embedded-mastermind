/**
 * @file task_joystick.h
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2025-08-14
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#ifndef __TASK_JOYSTICK_H__
#define __TASK_JOYSTICK_H__
#include "main.h"
#include "drivers.h"
#include "rtos_events.h"

#ifdef ECE353_FREERTOS

/**
 * @brief
 * Task that monitors the joystick and publishes movement events.
 * 
 * Periodically sends joystick read requests via Queue_Requests_Joystick
 * and sets EventGroup bits based on joystick direction changes.
 * 
 * @param arg Unused parameter
 */
void task_joystick(void *arg);

/**
 * @brief
 * Initialize the joystick task and create it with FreeRTOS.
 * 
 * @return bool True if successful, false otherwise
 */
bool task_joystick_init(void);

#endif
#endif /* __TASK_JOYSTICK_H__ */