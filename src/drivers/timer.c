/**
 * @file timer.c
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2024-08-14
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#include "timer.h"
#include <complex.h>

cy_rslt_t timer_init(cyhal_timer_t *timer_obj, cyhal_timer_cfg_t *timer_cfg, uint32_t ticks, void *Handler)
{
    cy_rslt_t rslt;

    timer_cfg->period = ticks;                     // Set the period for the timer
    timer_cfg->direction = CYHAL_TIMER_DIR_UP;     // Configure the timer to count up
    timer_cfg->is_compare = false;                 // Disable compare mode
    timer_cfg->is_continuous = true;               // Configure the timer to run continuously
    timer_cfg->value = 0;                          // Initialize the counter value to 0
    
    // Initialize the timer with the specified configuration
    rslt = cyhal_timer_init(timer_obj, NC, NULL);
    if (rslt != CY_RSLT_SUCCESS)
    {
        return rslt; // Return if initialization failed
    }   

    // Configure the timer interrupt
    rslt = cyhal_timer_configure(timer_obj, timer_cfg);
    if (rslt != CY_RSLT_SUCCESS)
    {
        return rslt; // Return if interrupt configuration failed
    }

    rslt = cyhal_timer_set_frequency(timer_obj, 100000000); // Set frequency to 100 MHz (example)
    if (rslt != CY_RSLT_SUCCESS)
    {
        return rslt; // Return if frequency setting failed
    }   

    cyhal_timer_register_callback(timer_obj, Handler, NULL); // Register the callback handler
    
    cyhal_timer_enable_event(timer_obj, CYHAL_TIMER_IRQ_TERMINAL_COUNT, 3, true); // Enable terminal count event
    
    rslt = cyhal_timer_start(timer_obj); // Start the timer
    if (rslt != CY_RSLT_SUCCESS)
    {
        return rslt; // Return if starting the timer failed
    }

    return rslt; // Return the result of the initialization
}