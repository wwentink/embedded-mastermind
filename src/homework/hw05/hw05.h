/**
 * @file hw05.h
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief HW05 Master Mind game definitions.
 * @version 0.1
 * @date 2026-04-24
 *
 * @copyright Copyright (c) 2026
 *
 */
#ifndef __HW05_H__
#define __HW05_H__

#include "main.h"

#if defined(HW05)

#include <stdint.h>
#include "drivers.h"
#include "master_mind_lib.h"
#include "rtos_events.h"
#include "task_buttons.h"
#include "task_console.h"
#include "task_cap_touch.h"
#include "task_eeprom.h"
#include "task_light_sensor.h"
#include "task_lcd.h"
#include "task_ipc.h"

#define TASK_SYSTEM_CONTROL_STACK_SIZE    (configMINIMAL_STACK_SIZE * 10U)
#define TASK_SYSTEM_CONTROL_PRIORITY      (tskIDLE_PRIORITY + 1U)

#define HW05_GAME_DIGIT_COUNT             (4U)
#define HW05_GAME_DIGIT_MAX               (7U)
#define HW05_EEPROM_HIGH_SCORE_ADDR       (0x0000U)
#define HW05_EEPROM_HIGH_SCORE_UNSET      (0xFFFFU)
#define HW05_LIGHT_THRESHOLD              (20000U)

typedef enum
{
	HW05_STATE_INIT_AND_SYNC = 0,
	HW05_STATE_SELECT_CIPHER,
	HW05_STATE_WAIT_FOR_PEER_READY,
	HW05_STATE_ACTIVE_BUILD_GUESS,
	HW05_STATE_ACTIVE_WAIT_FEEDBACK,
	HW05_STATE_PASSIVE_WAIT_GUESS,
	HW05_STATE_PASSIVE_WAIT_TURN_END_ACK,
	HW05_STATE_WAIT_GAME_OVER,
	HW05_STATE_GAME_OVER,
} hw05_state_t;

typedef enum
{
	HW05_ROLE_UNDECIDED = 0,
	HW05_ROLE_ACTIVE_FIRST,
	HW05_ROLE_PASSIVE_FIRST,
} hw05_role_t;

typedef enum
{
	HW05_THEME_DARK = 0,
	HW05_THEME_LIGHT = 1,
} hw05_theme_t;

typedef enum
{
	HW05_MSG_READY = IPC_CMD_GAME_READY,
	HW05_MSG_GUESS = IPC_CMD_GAME_GUESS,
	HW05_MSG_FEEDBACK = IPC_CMD_GAME_FEEDBACK,
	HW05_MSG_TURN_END_ACK = IPC_CMD_GAME_TURN_END_ACK,
	HW05_MSG_GAME_OVER = IPC_CMD_GAME_OVER,
	HW05_MSG_RESTART = IPC_CMD_GAME_RESTART,
} hw05_uart_message_type_t;

typedef struct __attribute__((packed))
{
	uint8_t digits[HW05_GAME_DIGIT_COUNT];
	uint8_t exact;
	uint8_t misplaced;
	uint8_t guess_count;
	uint8_t flags;
} hw05_uart_payload_t;

typedef struct __attribute__((packed))
{
	uint8_t start_byte;
	hw05_uart_message_type_t cmd;
	uint16_t sequence_num;
	hw05_uart_payload_t payload;
	uint8_t checksum;
} hw05_uart_packet_t;

/*****************************************************************************/
/* Function Definitions                                                      */
/*****************************************************************************/
void task_hw05_system_control(void *pvParameters);

#endif

#endif /* __HW05_H__ */