/**
 * @file leds.h
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2025-06-30
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#ifndef __LEDS_H__
#define __LEDS_H__

#include <stdio.h>
#include "cybsp.h"
#include "cyhal_gpio.h"
#include "ece353-pins.h"
#include "cyhal_pwm.h"

/* Enum for LED states */
typedef enum {
    LED_STATE_OFF,
    LED_STATE_ON
} ece353_led_state_t;

/* Enum for LEDs */
typedef enum {
    LED_RED,
    LED_GREEN,
    LED_BLUE
} ece353_led_t;

// Function to initialize the LEDs
cy_rslt_t leds_init_gpio(void);

// Function to set the state of a specific LED
void leds_set_state(ece353_led_t led, ece353_led_state_t state);

// Function that configures the RGB LED pins to be controlled by PWM
cy_rslt_t leds_init_pwm(
    cyhal_pwm_t *pwm_obj_red,
    cyhal_pwm_t *pwm_obj_green,
    cyhal_pwm_t *pwm_obj_blue
);


#endif