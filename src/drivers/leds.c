/**
 * @file leds.c
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2025-06-30
 * 
 * @copyright Copyright (c) 2025
 * 
 */

 #include "leds.h"
#include "cyhal.h"

/* Initialize the GPIO pins for the LEDs */
cy_rslt_t leds_init_gpio(void) {
    cy_rslt_t result = CY_RSLT_SUCCESS;

    // Initialize LED_RED
    result = cyhal_gpio_init(PIN_LED_RED, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, 0);
    if (result != CY_RSLT_SUCCESS) {
        return result;
    }

    // Initialize LED_GREEN
    result = cyhal_gpio_init(PIN_LED_GREEN, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, 0);
    if (result != CY_RSLT_SUCCESS) {
        return result;
    }

    // Initialize LED_BLUE
    result = cyhal_gpio_init(PIN_LED_BLUE, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, 0);
    if (result != CY_RSLT_SUCCESS) {
        return result;
    }

    return result;
}

/* Set the state of a specific LED fast */
void leds_set_state(ece353_led_t led, ece353_led_state_t state) {
    switch(led) {
        case LED_RED:
            if (state) {
                PORT_LED_RED->OUT |= MASK_LED_PIN_RED;
            } else {
                PORT_LED_RED->OUT &= ~MASK_LED_PIN_RED;
            }
            break;
        case LED_GREEN:
            if (state) {
                PORT_LED_GREEN->OUT |= MASK_LED_PIN_GREEN;
            } else {
                PORT_LED_GREEN->OUT &= ~MASK_LED_PIN_GREEN;
            }
            break;
        case LED_BLUE:
            if (state) {
                PORT_LED_BLUE->OUT |= MASK_LED_PIN_BLUE;
            } else {
                PORT_LED_BLUE->OUT &= ~MASK_LED_PIN_BLUE;
            }
            break;
    }
}

/* Configure the RGB LED pins to be controlled by PWM */
cy_rslt_t leds_init_pwm(
    cyhal_pwm_t *pwm_obj_red,
    cyhal_pwm_t *pwm_obj_green,
    cyhal_pwm_t *pwm_obj_blue
) {
    cy_rslt_t result = CY_RSLT_SUCCESS;
    result = cyhal_pwm_init(
        pwm_obj_red,
        PIN_LED_RED,
        NULL
    );

    if (result != CY_RSLT_SUCCESS) {
        return result;
    }

    result = cyhal_pwm_init(
        pwm_obj_green,
        PIN_LED_GREEN,
        NULL
    );

    if (result != CY_RSLT_SUCCESS) {
        return result;
    }

    result = cyhal_pwm_init(
        pwm_obj_blue,
        PIN_LED_BLUE,
        NULL
    );

    return result;
}