/**
 * @file buzzer.c
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2025-07-10
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include "buzzer.h"
#include "timer.h"

static cyhal_timer_t buzzer_timer;
static cyhal_timer_cfg_t buzzer_timer_cfg;

static void buzzer_timer_handler(void *handler_arg, cyhal_timer_event_t event)
{
    PORT_BUZZER->OUT_INV = MASK_BUZZER;
}

cy_rslt_t buzzer_init(float duty_cycle, uint32_t frequency)
{
    cy_rslt_t rslt;
    
    // Initialize buzzer GPIO as output
    rslt = cyhal_gpio_init(PIN_BUZZER, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, 0);
    if (rslt != CY_RSLT_SUCCESS)
    {
        return rslt;
    }
    
    // Calculate timer tick count (frequency is divided by 2 for toggle behavior)
    uint32_t tick_count = (100000000 / frequency) / 2;
    
    // Initialize timer
    rslt = timer_init(&buzzer_timer, &buzzer_timer_cfg, tick_count, buzzer_timer_handler);
    
    return rslt;
}

void buzzer_on(void)
{
    cyhal_timer_start(&buzzer_timer);
}

void buzzer_off(void)
{
    cyhal_timer_stop(&buzzer_timer);
    cyhal_gpio_write(PIN_BUZZER, 0);
}