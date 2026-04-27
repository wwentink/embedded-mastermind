/**
 * @file rtos_events.h
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2025-08-13
 * 
 * @copyright Copyright (c) 2025
 * 
 */

 #ifndef __RTOS_EVENTS_H__
 #define __RTOS_EVENTS_H__
 #include "main.h"
 
#ifdef ECE353_FREERTOS

/*******************************************************************************
* Event Group for system events.
 ******************************************************************************/
extern EventGroupHandle_t ECE353_RTOS_Events;

/*******************************************************************************
* Macros used to define the system events
******************************************************************************/
// Button events
#define EVENT_BUTTON_SW1       (1 << 5)    // Event bit for SW1 pressed
#define EVENT_BUTTON_SW2       (1 << 6)    // Event bit for SW2 pressed
#define EVENT_BUTTON_SW3       (1 << 7)    // Event bit for SW3 pressed

// Legacy button event names (for backward compatibility)
#define ECE353_EVENT_SW1_PRESSED   EVENT_BUTTON_SW1
#define ECE353_EVENT_SW2_PRESSED   EVENT_BUTTON_SW2
#define ECE353_EVENT_SW3_PRESSED   EVENT_BUTTON_SW3

// Joystick events
#define EVENT_JOYSTICK_UP          (1 << 0)    // Joystick moved UP
#define EVENT_JOYSTICK_DOWN        (1 << 1)    // Joystick moved DOWN
#define EVENT_JOYSTICK_LEFT        (1 << 2)    // Joystick moved LEFT
#define EVENT_JOYSTICK_RIGHT       (1 << 3)    // Joystick moved RIGHT
#define EVENT_JOYSTICK_CENTER      (1 << 4)    // Joystick returned to CENTER

// IPC events
#define ECE353_RTOS_EVENTS_IPC_ACK_RECEIVED   (1 << 8)   // Event bit for IPC ACK received
#define ECE353_RTOS_EVENTS_IPC_DISCOVERY_RX    (1 << 9)   // Event bit when a discovery message is received
#define ECE353_RTOS_EVENTS_IPC_GAME_READY_RX   (1 << 10)  // Event bit when a game READY message is received
#define ECE353_RTOS_EVENTS_IPC_GAME_GUESS_RX   (1 << 11)  // Event bit when a game GUESS message is received
#define ECE353_RTOS_EVENTS_IPC_GAME_FEEDBACK_RX (1 << 12) // Event bit when a game FEEDBACK message is received
#define ECE353_RTOS_EVENTS_IPC_GAME_TURN_END_ACK_RX (1 << 13) // Event bit when a TURN_END_ACK message is received
#define ECE353_RTOS_EVENTS_IPC_GAME_OVER_RX    (1 << 14)  // Event bit when a GAME_OVER message is received
#define ECE353_RTOS_EVENTS_IPC_GAME_RESTART_RX  (1 << 15)  // Event bit when a RESTART message is received

#endif // ECE353_FREERTOS

#endif // __RTOS_EVENTS_H__