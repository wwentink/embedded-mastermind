 /**
 * @file hw02.h
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2025-10-08
 * 
 * @copyright Copyright (c) 2025
 * 
 */
 #ifndef __HW02_H__
 #define __HW02_H__

 #include "main.h"

 #if defined(HW02)

#include "drivers.h"
#include "rtos_events.h"
#include "task_lcd.h"
#include "task_buttons.h"
#include "task_joystick.h"
#include "task_device_gatekeeper.h"
#include "devices.h"
#include "master_mind_lib.h"
#include <string.h>

#define  TASK_SYSTEM_CONTROL_STACK_SIZE    (configMINIMAL_STACK_SIZE*5)
#define  TASK_SYSTEM_CONTROL_PRIORITY      (tskIDLE_PRIORITY + 1U)  

/* Global event group and queues (declared in hw02.c) */
extern EventGroupHandle_t ECE353_RTOS_Events;
extern QueueHandle_t Queue_Requests_Joystick;
extern QueueHandle_t Queue_LCD_Request;

/*****************************************************************************/
/* Function Definitions                                                      */
/*****************************************************************************/
void task_hw02_system_control(void *pvParameters);


 #endif

 #endif /* __HW02_H__ */