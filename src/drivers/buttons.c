/**
 * @file buttons.c
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2025-06-30
 * 
 * @copyright Copyright (c) 2025
 * 
 */

 #include "buttons.h"

#include "buttons.h"

/* Store previous states for edge detection */
static button_state_t prev_state_sw1 = BUTTON_STATE_HIGH;
static button_state_t prev_state_sw2 = BUTTON_STATE_HIGH;
static button_state_t prev_state_sw3 = BUTTON_STATE_HIGH;

/* Initialize the GPIO pins for the buttons */
cy_rslt_t buttons_init_gpio(void) {
    cy_rslt_t result = CY_RSLT_SUCCESS;

    // Initialize SW1
    result = cyhal_gpio_init(PIN_BUTTON_SW1, CYHAL_GPIO_DIR_INPUT, CYHAL_GPIO_DRIVE_NONE, 0);
    if (result != CY_RSLT_SUCCESS) {
        return result;
    }

    // Initialize SW2
    result = cyhal_gpio_init(PIN_BUTTON_SW2, CYHAL_GPIO_DIR_INPUT, CYHAL_GPIO_DRIVE_NONE, 0);
    if (result != CY_RSLT_SUCCESS) {
        return result;
    }

    // Initialize SW3
    result = cyhal_gpio_init(PIN_BUTTON_SW3, CYHAL_GPIO_DIR_INPUT, CYHAL_GPIO_DRIVE_NONE, 0);
    if (result != CY_RSLT_SUCCESS) {
        return result;
    }

    return result;
}

/* Initialize the timer for button debouncing */
cy_rslt_t buttons_init_timer(void) {
    // Implementation would go here
    return CY_RSLT_SUCCESS;
}

/* Read the state of a specific button */
button_state_t buttons_get_state(ece353_button_t button) {
    button_state_t current_state;
    button_state_t return_state;

    switch(button) {
        case BUTTON_SW1:
            current_state = (PORT_BUTTON_SW1->IN & MASK_BUTTON_PIN_SW1) ? BUTTON_STATE_HIGH : BUTTON_STATE_LOW;
            if (prev_state_sw1 == BUTTON_STATE_HIGH && current_state == BUTTON_STATE_LOW) {
                return_state = BUTTON_STATE_FALLING_EDGE;
            } else if (prev_state_sw1 == BUTTON_STATE_LOW && current_state == BUTTON_STATE_HIGH) {
                return_state = BUTTON_STATE_RISING_EDGE;
            } else {
                return_state = current_state;
            }
            prev_state_sw1 = current_state;
            return return_state;

        case BUTTON_SW2:
            current_state = (PORT_BUTTON_SW2->IN & MASK_BUTTON_PIN_SW2) ? BUTTON_STATE_HIGH : BUTTON_STATE_LOW;
            if (prev_state_sw2 == BUTTON_STATE_HIGH && current_state == BUTTON_STATE_LOW) {
                return_state = BUTTON_STATE_FALLING_EDGE;
            } else if (prev_state_sw2 == BUTTON_STATE_LOW && current_state == BUTTON_STATE_HIGH) {
                return_state = BUTTON_STATE_RISING_EDGE;
            } else {
                return_state = current_state;
            }
            prev_state_sw2 = current_state;
            return return_state;

        case BUTTON_SW3:
            current_state = (PORT_BUTTON_SW3->IN & MASK_BUTTON_PIN_SW3) ? BUTTON_STATE_HIGH : BUTTON_STATE_LOW;
            if (prev_state_sw3 == BUTTON_STATE_HIGH && current_state == BUTTON_STATE_LOW) {
                return_state = BUTTON_STATE_FALLING_EDGE;
            } else if (prev_state_sw3 == BUTTON_STATE_LOW && current_state == BUTTON_STATE_HIGH) {
                return_state = BUTTON_STATE_RISING_EDGE;
            } else {
                return_state = current_state;
            }
            prev_state_sw3 = current_state;
            return return_state;

        default:
            return BUTTON_STATE_LOW;
    }
}