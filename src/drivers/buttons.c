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

/* Read the state of a specific button */
// button_state_t buttons_get_state(ece353_button_t button) {
//     button_state_t current_state;
//     button_state_t return_state;

//     switch(button) {
//         case BUTTON_SW1:
//             current_state = (PORT_BUTTON_SW1->IN & MASK_BUTTON_PIN_SW1) ? BUTTON_STATE_HIGH : BUTTON_STATE_LOW;
//             if (prev_state_sw1 == BUTTON_STATE_HIGH && current_state == BUTTON_STATE_LOW) {
//                 return_state = BUTTON_STATE_FALLING_EDGE;
//             } else if (prev_state_sw1 == BUTTON_STATE_LOW && current_state == BUTTON_STATE_HIGH) {
//                 return_state = BUTTON_STATE_RISING_EDGE;
//             } else {
//                 return_state = current_state;
//             }
//             prev_state_sw1 = current_state;
//             return return_state;

//         case BUTTON_SW2:
//             current_state = (PORT_BUTTON_SW2->IN & MASK_BUTTON_PIN_SW2) ? BUTTON_STATE_HIGH : BUTTON_STATE_LOW;
//             if (prev_state_sw2 == BUTTON_STATE_HIGH && current_state == BUTTON_STATE_LOW) {
//                 return_state = BUTTON_STATE_FALLING_EDGE;
//             } else if (prev_state_sw2 == BUTTON_STATE_LOW && current_state == BUTTON_STATE_HIGH) {
//                 return_state = BUTTON_STATE_RISING_EDGE;
//             } else {
//                 return_state = current_state;
//             }
//             prev_state_sw2 = current_state;
//             return return_state;

//         case BUTTON_SW3:
//             current_state = (PORT_BUTTON_SW3->IN & MASK_BUTTON_PIN_SW3) ? BUTTON_STATE_HIGH : BUTTON_STATE_LOW;
//             if (prev_state_sw3 == BUTTON_STATE_HIGH && current_state == BUTTON_STATE_LOW) {
//                 return_state = BUTTON_STATE_FALLING_EDGE;
//             } else if (prev_state_sw3 == BUTTON_STATE_LOW && current_state == BUTTON_STATE_HIGH) {
//                 return_state = BUTTON_STATE_RISING_EDGE;
//             } else {
//                 return_state = current_state;
//             }
//             prev_state_sw3 = current_state;
//             return return_state;

//         default:
//             return BUTTON_STATE_LOW;
//     }
// }

static cyhal_timer_t button_timer;
static cyhal_timer_cfg_t button_timer_cfg;

/* TCPWM interrupt handler for button debouncing */
/* Debounces buttons for 25ms and only reports pressed once per button press */
static void button_timer_handler(void *arg, cyhal_timer_event_t event) {
    static uint8_t button_counts[3] = {0, 0, 0};
    static uint8_t button_pressed[3] = {0, 0, 0};  /* Track if button press event was reported */

    uint8_t sw1 = PORT_BUTTON_SW1->IN & MASK_BUTTON_PIN_SW1;
    uint8_t sw2 = PORT_BUTTON_SW2->IN & MASK_BUTTON_PIN_SW2;
    uint8_t sw3 = PORT_BUTTON_SW3->IN & MASK_BUTTON_PIN_SW3;

    /* Button 1 debouncing and edge detection */
    if (sw1 == 0) {
        button_counts[0]++;
        /* Debounce for 25ms (5 * 5ms intervals) */
        if (button_counts[0] == 5 && button_pressed[0] == 0) {
            ECE353_Events.sw1 = 1;
            button_pressed[0] = 1;  /* Mark that press event was reported */
        }
    } else {
        button_counts[0] = 0;
        button_pressed[0] = 0;  /* Reset when button is released */
    }

    /* Button 2 debouncing and edge detection */
    if (sw2 == 0) {
        button_counts[1]++;
        /* Debounce for 25ms (5 * 5ms intervals) */
        if (button_counts[1] == 5 && button_pressed[1] == 0) {
            ECE353_Events.sw2 = 1;
            button_pressed[1] = 1;  /* Mark that press event was reported */
        }
    } else {
        button_counts[1] = 0;
        button_pressed[1] = 0;  /* Reset when button is released */
    }

    /* Button 3 debouncing and edge detection */
    if (sw3 == 0) {
        button_counts[2]++;
        /* Debounce for 25ms (5 * 5ms intervals) */
        if (button_counts[2] == 5 && button_pressed[2] == 0) {
            ECE353_Events.sw3 = 1;
            button_pressed[2] = 1;  /* Mark that press event was reported */
        }
    } else {
        button_counts[2] = 0;
        button_pressed[2] = 0;  /* Reset when button is released */
    }

}

/* Initialize the timer for button debouncing */
cy_rslt_t buttons_init_timer(void) {
 
    return timer_init(&button_timer, &button_timer_cfg, 5000, button_timer_handler); // Timer period set to 5ms
}