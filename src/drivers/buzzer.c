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

static cyhal_pwm_t buzzer_pwm;

cy_rslt_t buzzer_init(float duty_cycle, uint32_t frequency)
{
    cy_rslt_t rslt;
    
    // Initialize buzzer PWM
    rslt = cyhal_pwm_init(&buzzer_pwm, PIN_BUZZER, NULL);
    if (rslt != CY_RSLT_SUCCESS)
    {
        return rslt;
    }
    
    // Set duty cycle and frequency
    rslt = cyhal_pwm_set_duty_cycle(&buzzer_pwm, duty_cycle, frequency);
    if (rslt != CY_RSLT_SUCCESS)
    {
        return rslt;
    }
    
    return rslt;
}

void buzzer_on(void)
{
    cyhal_pwm_start(&buzzer_pwm);
}

void buzzer_off(void)
{
    cyhal_pwm_stop(&buzzer_pwm);
}