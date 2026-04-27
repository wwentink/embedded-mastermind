/**
 * @file hw05.c
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief HW05 Master Mind game control loop.
 * @version 0.1
 * @date 2026-04-24
 *
 * @copyright Copyright (c) 2026
 *
 */
#include "hw05.h"

#include <stdio.h>
#include <string.h>

#if defined(HW05)

/*****************************************************************************/
/* Global Variables                                                          */
/*****************************************************************************/
char APP_DESCRIPTION[] = "ECE353 S26 HW05";
cyhal_i2c_t *I2C_Monarch_Obj;
cyhal_spi_t *SPI_Monarch_Obj;
SemaphoreHandle_t I2C_Monarch_Semaphore;
SemaphoreHandle_t SPI_Monarch_Semaphore;
EventGroupHandle_t ECE353_RTOS_Events = NULL;
QueueHandle_t xQueue_Request_LCD = NULL;

static QueueHandle_t Queue_Cap_Touch_Responses = NULL;
static QueueHandle_t Queue_EEPROM_Responses = NULL;
static QueueHandle_t Queue_Light_Sensor_Responses = NULL;
static bool hw05_banner_printed = false;

#define HW05_BLANK_DIGIT  (0xFFU)
#define HW05_TILE_LIGHT_BLUE (0xAEDC)
#define HW05_TILE_PURPLE     (0x780F)
#define HW05_TILE_PINK       (0xFB1A)

/*****************************************************************************/
/* Local Helpers                                                             */
/*****************************************************************************/
static void hw05_die(const char *message)
{
    printf("%s\n\r", message);
    for(int i = 0; i < 100000; i++) {}
    CY_ASSERT(0);
}

static void hw05_reset_digits(uint8_t digits[HW05_GAME_DIGIT_COUNT])
{
    for(uint32_t i = 0; i < HW05_GAME_DIGIT_COUNT; i++)
    {
        digits[i] = HW05_BLANK_DIGIT;
    }
}

static void hw05_copy_digits(uint8_t *dst, const uint8_t *src)
{
    for(uint32_t i = 0; i < HW05_GAME_DIGIT_COUNT; i++)
    {
        dst[i] = src[i];
    }
}

static void hw05_format_high_score(uint16_t score, char *buffer, size_t buffer_size)
{
    if((buffer == NULL) || (buffer_size == 0U))
    {
        return;
    }

    if(score == HW05_EEPROM_HIGH_SCORE_UNSET)
    {
        (void)snprintf(buffer, buffer_size, "--");
    }
    else
    {
        (void)snprintf(buffer, buffer_size, "%u", (unsigned int)score);
    }
}

static uint16_t hw05_theme_bg(hw05_theme_t theme)
{
    return (theme == HW05_THEME_LIGHT) ? LCD_COLOR_WHITE : LCD_COLOR_BLACK;
}

static uint16_t hw05_tile_bg_color(hw05_theme_t theme, lcd_row_t row)
{
    if(row == LCD_TILE_ROW_CYPHER)
    {
        return HW05_TILE_PINK;
    }

    return (theme == HW05_THEME_LIGHT) ? HW05_TILE_LIGHT_BLUE : HW05_TILE_PURPLE;
}

static uint16_t hw05_tile_fg_color(hw05_theme_t theme, lcd_row_t row)
{
    if(row == LCD_TILE_ROW_CYPHER)
    {
        return LCD_COLOR_WHITE;
    }

    return (theme == HW05_THEME_LIGHT) ? LCD_COLOR_BLACK : LCD_COLOR_WHITE;
}

static bool hw05_send_lcd_request(const lcd_msg_t *msg)
{
    lcd_msg_request_t request;

    if((msg == NULL) || (xQueue_Request_LCD == NULL))
    {
        return false;
    }

    request.msg = *msg;
    request.return_queue = NULL;

    return (xQueueSend(xQueue_Request_LCD, &request, portMAX_DELAY) == pdPASS);
}

static bool hw05_lcd_clear(hw05_theme_t theme)
{
    lcd_msg_t msg = {0};
    msg.command = LCD_CMD_CLEAR_SCREEN;
    msg.payload.tile.color_bg = hw05_theme_bg(theme);
    return hw05_send_lcd_request(&msg);
}

static bool hw05_lcd_print(const char *text)
{
    lcd_msg_t msg = {0};

    if(text == NULL)
    {
        return false;
    }

    msg.command = LCD_CMD_PRINT_MESSAGE;
    (void)snprintf(msg.payload.message, sizeof(msg.payload.message), "%s", text);

    return hw05_send_lcd_request(&msg);
}

static bool hw05_lcd_draw_tile(lcd_row_t row, uint8_t col, uint8_t number, bool inverted, hw05_theme_t theme)
{
    lcd_msg_t msg = {0};
    lcd_tile_t *tile = &msg.payload.tile;

    if(col >= 4U)
    {
        return false;
    }

    msg.command = inverted ? LCD_CMD_DRAW_TILE_INVERTED : LCD_CMD_DRAW_TILE;
    tile->row = row;
    tile->col = col;
    tile->number = number;
    tile->color_fg = hw05_tile_fg_color(theme, row);
    tile->color_bg = hw05_tile_bg_color(theme, row);

    return hw05_send_lcd_request(&msg);
}

static void hw05_render_palette(hw05_theme_t theme)
{
    for(uint8_t digit = 0; digit < 4U; digit++)
    {
        (void)hw05_lcd_draw_tile(LCD_TILE_ROW_NUM_0_3, digit, digit, false, theme);
    }

    for(uint8_t digit = 4U; digit < 8U; digit++)
    {
        (void)hw05_lcd_draw_tile(LCD_TILE_ROW_NUM_4_7, digit - 4U, digit, false, theme);
    }
}

static void hw05_render_entry_row(
    hw05_theme_t theme,
    const uint8_t digits[HW05_GAME_DIGIT_COUNT],
    uint8_t digit_count,
    uint8_t cursor
)
{
    for(uint8_t col = 0; col < HW05_GAME_DIGIT_COUNT; col++)
    {
        const bool filled = (col < digit_count);
        const bool highlight = (col == cursor) && (digit_count < HW05_GAME_DIGIT_COUNT);
        const uint8_t number = filled ? digits[col] : HW05_BLANK_DIGIT;

        (void)hw05_lcd_draw_tile(LCD_TILE_ROW_CYPHER, col, number, highlight, theme);
    }
}

static void hw05_render_input_screen(
    hw05_theme_t theme,
    const char *status,
    const uint8_t digits[HW05_GAME_DIGIT_COUNT],
    uint8_t digit_count,
    uint8_t cursor,
    bool show_palette
)
{
    (void)hw05_lcd_clear(theme);
    (void)hw05_lcd_print(status);
    hw05_render_entry_row(theme, digits, digit_count, cursor);

    if(show_palette)
    {
        hw05_render_palette(theme);
    }
}

static void hw05_render_message_screen(hw05_theme_t theme, const char *status)
{
    (void)hw05_lcd_clear(theme);
    (void)hw05_lcd_print(status);
}

static bool hw05_read_touch(uint16_t *screen_x, uint16_t *screen_y)
{
    device_request_msg_t request = {0};
    device_response_msg_t response = {0};

    if((screen_x == NULL) || (screen_y == NULL) || (Queue_Cap_Touch_Responses == NULL))
    {
        return false;
    }

    request.device = DEVICE_CAP_TOUCH;
    request.operation = DEVICE_OP_READ;
    request.response_queue = Queue_Cap_Touch_Responses;

    if(xQueueSend(Queue_Request_Cap_Touch, &request, portMAX_DELAY) != pdPASS)
    {
        return false;
    }

    if(xQueueReceive(Queue_Cap_Touch_Responses, &response, pdMS_TO_TICKS(100)) != pdPASS)
    {
        return false;
    }

    if(response.status != DEVICE_OPERATION_STATUS_READ_SUCCESS)
    {
        return false;
    }

    *screen_x = response.payload.cap_touch[0];
    *screen_y = response.payload.cap_touch[1];
    return true;
}

static bool hw05_touch_to_digit(uint16_t screen_x, uint16_t screen_y, uint8_t *digit)
{
    if(digit == NULL)
    {
        return false;
    }

    for(uint8_t candidate = 0; candidate < 8U; candidate++)
    {
        lcd_row_t row = (candidate < 4U) ? LCD_TILE_ROW_NUM_0_3 : LCD_TILE_ROW_NUM_4_7;
        uint8_t col = candidate & 0x03U;
        lcd_rect_t rect;

        if(!lcd_tile_rect(&rect, row, col))
        {
            continue;
        }

        if((screen_x >= (uint16_t)(rect.cx - (rect.w / 2))) &&
           (screen_x <= (uint16_t)(rect.cx + (rect.w / 2))) &&
           (screen_y >= (uint16_t)(rect.cy - (rect.h / 2))) &&
           (screen_y <= (uint16_t)(rect.cy + (rect.h / 2))))
        {
            *digit = candidate;
            return true;
        }
    }

    return false;
}

static bool hw05_read_high_score(uint16_t *score)
{
    uint8_t low_byte = 0U;
    uint8_t high_byte = 0U;
    uint8_t magic = 0U;
    uint16_t value = HW05_EEPROM_HIGH_SCORE_UNSET;

    if(score == NULL)
    {
        return false;
    }

    if(!system_sensors_eeprom_read(Queue_EEPROM_Responses, HW05_EEPROM_HIGH_SCORE_MAGIC_ADDR, &magic))
    {
        return false;
    }

    if(!system_sensors_eeprom_read(Queue_EEPROM_Responses, HW05_EEPROM_HIGH_SCORE_ADDR, &low_byte))
    {
        return false;
    }

    if(!system_sensors_eeprom_read(Queue_EEPROM_Responses, HW05_EEPROM_HIGH_SCORE_ADDR + 1U, &high_byte))
    {
        return false;
    }

    value = ((uint16_t)high_byte << 8) | low_byte;

    if(value == HW05_EEPROM_HIGH_SCORE_UNSET)
    {
        *score = HW05_EEPROM_HIGH_SCORE_UNSET;
        return true;
    }

    /*
     * Accept legacy records that only stored the 16-bit score value.
     * The magic marker is used for robustness, but should not erase valid prior data.
     */
    if((magic != HW05_EEPROM_HIGH_SCORE_MAGIC) &&
       (value >= HW05_HIGH_SCORE_MIN_VALID) &&
       (value <= HW05_HIGH_SCORE_MAX_VALID))
    {
        *score = value;
        return true;
    }

    if((value < HW05_HIGH_SCORE_MIN_VALID) || (value > HW05_HIGH_SCORE_MAX_VALID))
    {
        *score = HW05_EEPROM_HIGH_SCORE_UNSET;
        return true;
    }

    *score = value;
    return true;
}

static bool hw05_write_high_score(uint16_t score)
{
    uint8_t low_byte = (uint8_t)(score & 0xFFU);
    uint8_t high_byte = (uint8_t)((score >> 8) & 0xFFU);
    uint16_t verify_score = 0U;

    if(!system_sensors_eeprom_write(Queue_EEPROM_Responses, HW05_EEPROM_HIGH_SCORE_ADDR, low_byte))
    {
        return false;
    }

    if(!system_sensors_eeprom_write(Queue_EEPROM_Responses, HW05_EEPROM_HIGH_SCORE_ADDR + 1U, high_byte))
    {
        return false;
    }

    if(!system_sensors_eeprom_write(
        Queue_EEPROM_Responses,
        HW05_EEPROM_HIGH_SCORE_MAGIC_ADDR,
        HW05_EEPROM_HIGH_SCORE_MAGIC
    ))
    {
        return false;
    }

    if(!hw05_read_high_score(&verify_score))
    {
        return false;
    }

    return (verify_score == score);
}

static bool hw05_read_ambient_light(uint16_t *ambient_light)
{
    return system_sensors_get_light(Queue_Light_Sensor_Responses, ambient_light);
}

static hw05_theme_t hw05_update_theme_from_light(uint16_t ambient_light, hw05_theme_t current_theme)
{
    if((current_theme == HW05_THEME_DARK) &&
       (ambient_light > (HW05_LIGHT_THRESHOLD + HW05_LIGHT_HYSTERESIS)))
    {
        return HW05_THEME_LIGHT;
    }

    if((current_theme == HW05_THEME_LIGHT) &&
       (ambient_light + HW05_LIGHT_HYSTERESIS < HW05_LIGHT_THRESHOLD))
    {
        return HW05_THEME_DARK;
    }

    return current_theme;
}

static void hw05_clear_ipc_sync_state(void)
{
    xEventGroupClearBits(
        ECE353_RTOS_Events,
        ECE353_RTOS_EVENTS_IPC_ACK_RECEIVED |
        ECE353_RTOS_EVENTS_IPC_DISCOVERY_RX |
        ECE353_RTOS_EVENTS_IPC_GAME_READY_RX |
        ECE353_RTOS_EVENTS_IPC_GAME_GUESS_RX |
        ECE353_RTOS_EVENTS_IPC_GAME_FEEDBACK_RX |
        ECE353_RTOS_EVENTS_IPC_GAME_TURN_END_ACK_RX |
        ECE353_RTOS_EVENTS_IPC_GAME_OVER_RX |
        ECE353_RTOS_EVENTS_IPC_GAME_RESTART_RX
    );

    ipc_reset_link_state();
}

static bool hw05_consume_rx_packet(ipc_cmd_t expected_cmd, ipc_packet_t *packet)
{
    bool valid = false;

    if(packet == NULL)
    {
        return false;
    }

    taskENTER_CRITICAL();
    valid = IPC_Last_Rx_Packet_Valid && (IPC_Last_Rx_Packet.cmd == expected_cmd);
    if(valid)
    {
        *packet = IPC_Last_Rx_Packet;
        IPC_Last_Rx_Packet_Valid = false;
    }
    taskEXIT_CRITICAL();

    return valid;
}

static void hw05_build_ready_payload(uint8_t ready_digits[HW05_GAME_DIGIT_COUNT])
{
    if(ready_digits == NULL)
    {
        return;
    }

    /*
     * Do not leak the local cipher over UART.
     * READY packets only communicate synchronization state.
     */
    for(uint8_t i = 0; i < HW05_GAME_DIGIT_COUNT; i++)
    {
        ready_digits[i] = 0U;
    }
}

static bool hw05_evaluate_guess(
    const uint8_t secret[HW05_GAME_DIGIT_COUNT],
    const uint8_t guess[HW05_GAME_DIGIT_COUNT],
    uint8_t *exact,
    uint8_t *misplaced
)
{
    bool secret_used[HW05_GAME_DIGIT_COUNT] = {false, false, false, false};
    bool guess_used[HW05_GAME_DIGIT_COUNT] = {false, false, false, false};

    if((exact == NULL) || (misplaced == NULL))
    {
        return false;
    }

    *exact = 0U;
    *misplaced = 0U;

    for(uint8_t i = 0; i < HW05_GAME_DIGIT_COUNT; i++)
    {
        if(guess[i] == secret[i])
        {
            (*exact)++;
            secret_used[i] = true;
            guess_used[i] = true;
        }
    }

    for(uint8_t i = 0; i < HW05_GAME_DIGIT_COUNT; i++)
    {
        if(guess_used[i])
        {
            continue;
        }

        for(uint8_t j = 0; j < HW05_GAME_DIGIT_COUNT; j++)
        {
            if(!secret_used[j] && (guess[i] == secret[j]))
            {
                (*misplaced)++;
                secret_used[j] = true;
                break;
            }
        }
    }

    return true;
}

static void hw05_render_current_state(
    hw05_state_t state,
    hw05_theme_t theme,
    const uint8_t *entry,
    uint8_t entry_count,
    uint8_t cursor,
    uint16_t high_score,
    uint16_t local_guess_count,
    uint16_t peer_guess_count,
    uint8_t exact,
    uint8_t misplaced,
    bool passive_win
)
{
    char status[128] = {0};
    char high_score_text[8] = {0};

    hw05_format_high_score(high_score, high_score_text, sizeof(high_score_text));

    switch(state)
    {
        case HW05_STATE_INIT_AND_SYNC:
            (void)snprintf(status, sizeof(status), "SYNC STAGE Record:%s", high_score_text);
            hw05_render_message_screen(theme, status);
            break;

        case HW05_STATE_SELECT_CIPHER:
            (void)snprintf(status, sizeof(status), "CHOOSE CIPHER Record:%s", high_score_text);
            hw05_render_input_screen(theme, status, entry, entry_count, cursor, true);
            break;

        case HW05_STATE_WAIT_FOR_PEER_READY:
            (void)snprintf(status, sizeof(status), "SYNC CIPHER Record:%s", high_score_text);
            hw05_render_input_screen(theme, status, entry, entry_count, cursor, true);
            break;

        case HW05_STATE_ACTIVE_BUILD_GUESS:
            (void)snprintf(status, sizeof(status), "ACTIVE #%u", (unsigned int)(local_guess_count + 1U));
            hw05_render_input_screen(theme, status, entry, entry_count, cursor, true);
            break;

        case HW05_STATE_ACTIVE_WAIT_FEEDBACK:
        case HW05_STATE_ACTIVE_VIEW_FEEDBACK:
            (void)snprintf(status, sizeof(status), "Feedback\n\n\n\nExact:%u Misplaced:%u\nPress SW1 to continue", exact, misplaced);
            hw05_render_input_screen(theme, status, entry, HW05_GAME_DIGIT_COUNT, HW05_GAME_DIGIT_COUNT, false);
            break;

        case HW05_STATE_PASSIVE_WAIT_GUESS:
            (void)snprintf(status, sizeof(status), "PASSIVE WAIT\n\n\n\nWaiting for opponent...");
            hw05_render_input_screen(theme, status, entry, entry_count, cursor, false);
            break;

        case HW05_STATE_PASSIVE_WAIT_TURN_END_ACK:
            (void)snprintf(status, sizeof(status), "Swap\n\n\n\nExact:%u Misplaced:%u\nWaiting for opponent...", exact, misplaced);
            hw05_render_input_screen(theme, status, entry, HW05_GAME_DIGIT_COUNT, HW05_GAME_DIGIT_COUNT, false);
            break;

        case HW05_STATE_WAIT_GAME_OVER:
            (void)snprintf(status, sizeof(status), "WAIT OVER");
            hw05_render_message_screen(theme, status);
            break;

        case HW05_STATE_GAME_OVER:
        {
            const char *result = passive_win ? "LOSER!" : "WINNER!";
            
            (void)snprintf(
                status,
                sizeof(status),
                "%s\nGUESSES YOU:%u PEER:%u\nPress SW2 to play again",
                result,
                (unsigned int)local_guess_count,
                (unsigned int)peer_guess_count
            );
            hw05_render_message_screen(theme, status);
            break;
        }

        default:
            (void)snprintf(status, sizeof(status), "HW05");
            hw05_render_message_screen(theme, status);
            break;
    }
}

static void hw05_reset_round_state(
    uint8_t *entry_digits,
    uint8_t *entry_count,
    uint8_t *cursor,
    uint8_t last_exact,
    uint8_t last_misplaced
)
{
    (void)last_exact;
    (void)last_misplaced;

    if(entry_digits != NULL)
    {
        hw05_reset_digits(entry_digits);
    }

    if(entry_count != NULL)
    {
        *entry_count = 0U;
    }

    if(cursor != NULL)
    {
        *cursor = 0U;
    }
}

static void hw05_reset_game(
    hw05_state_t *state,
    uint8_t *local_cipher,
    uint8_t *peer_cipher,
    uint8_t *entry_digits,
    uint8_t *entry_count,
    uint8_t *cursor,
    uint16_t *local_guess_count,
    uint16_t *peer_guess_count,
    uint8_t *exact,
    uint8_t *misplaced,
    bool *local_ready_sent,
    bool *peer_ready_seen,
    bool *peer_ready_seen_before_local_ready,
    bool *local_goes_first
)
{
    if(state != NULL)
    {
        *state = HW05_STATE_INIT_AND_SYNC;
    }

    if(local_cipher != NULL)
    {
        hw05_reset_digits(local_cipher);
    }

    if(peer_cipher != NULL)
    {
        hw05_reset_digits(peer_cipher);
    }

    hw05_reset_round_state(entry_digits, entry_count, cursor, 0U, 0U);

    if(local_guess_count != NULL)
    {
        *local_guess_count = 0U;
    }

    if(peer_guess_count != NULL)
    {
        *peer_guess_count = 0U;
    }

    if(exact != NULL)
    {
        *exact = 0U;
    }

    if(misplaced != NULL)
    {
        *misplaced = 0U;
    }

    if(local_ready_sent != NULL)
    {
        *local_ready_sent = false;
    }

    if(peer_ready_seen != NULL)
    {
        *peer_ready_seen = false;
    }

    if(peer_ready_seen_before_local_ready != NULL)
    {
        *peer_ready_seen_before_local_ready = false;
    }

    if(local_goes_first != NULL)
    {
        *local_goes_first = false;
    }

    hw05_clear_ipc_sync_state();
}

/*****************************************************************************/
/* Function Definitions                                                      */
/*****************************************************************************/
void app_init_hw(void)
{
    cy_rslt_t rslt;

    console_init();

    if(!hw05_banner_printed)
    {
        printf("\x1b[30m");
        printf("\x1b[2J\x1b[;H");
        printf("**************************************************\n\r");
        printf("* %s\n\r", APP_DESCRIPTION);
        printf("* Date: %s\n\r", __DATE__);
        printf("* Time: %s\n\r", __TIME__);
        printf("* Name: %s\n\r", NAME);
        printf("**************************************************\n\r");
        hw05_banner_printed = true;
    }

    I2C_Monarch_Obj = i2c_init(PIN_I2C_SDA, PIN_I2C_SCL);
    if(I2C_Monarch_Obj == NULL)
    {
        hw05_die("I2C initialization failed!");
    }

    SPI_Monarch_Obj = spi_init(PIN_SPI_MOSI, PIN_SPI_MISO, PIN_SPI_CLK);
    if(SPI_Monarch_Obj == NULL)
    {
        hw05_die("SPI initialization failed!");
    }

    rslt = lcd_initialize();
    if(rslt != CY_RSLT_SUCCESS)
    {
        hw05_die("LCD initialization failed!");
    }
}

void task_hw05_system_control(void *pvParameters)
{
    (void)pvParameters;

    hw05_state_t state = HW05_STATE_INIT_AND_SYNC;
    hw05_theme_t theme = HW05_THEME_DARK;
    uint8_t local_cipher[HW05_GAME_DIGIT_COUNT];
    uint8_t peer_cipher[HW05_GAME_DIGIT_COUNT];
    uint8_t ready_payload[HW05_GAME_DIGIT_COUNT];
    uint8_t entry_digits[HW05_GAME_DIGIT_COUNT];
    uint8_t last_guess_digits[HW05_GAME_DIGIT_COUNT];
    uint16_t high_score = HW05_EEPROM_HIGH_SCORE_UNSET;
    uint16_t ambient_light = 0U;
    uint16_t local_guess_count = 0U;
    uint16_t peer_guess_count = 0U;
    uint8_t entry_count = 0U;
    uint8_t entry_cursor = 0U;
    uint8_t exact = 0U;
    uint8_t misplaced = 0U;
    bool local_ready_sent = false;
    bool peer_ready_seen = false;
    bool peer_ready_seen_before_local_ready = false;
    bool local_goes_first = false;
    bool passive_win = false;
    bool ui_dirty = true;
    bool sync_complete = false;
    bool touch_latched = false;
    uint16_t tx_sequence = 1U;
    TickType_t next_discovery_tick = 0U;
    TickType_t startup_tick = xTaskGetTickCount();

    hw05_reset_digits(local_cipher);
    hw05_reset_digits(peer_cipher);
    hw05_build_ready_payload(ready_payload);
    hw05_reset_digits(entry_digits);
    hw05_reset_digits(last_guess_digits);
    hw05_clear_ipc_sync_state();

    if(!hw05_read_high_score(&high_score))
    {
        high_score = HW05_EEPROM_HIGH_SCORE_UNSET;
    }

    hw05_render_current_state(
        state,
        theme,
        entry_digits,
        entry_count,
        entry_cursor,
        high_score,
        local_guess_count,
        peer_guess_count,
        exact,
        misplaced,
        passive_win
    );
    ui_dirty = false;

    while(1)
    {
        EventBits_t events = xEventGroupWaitBits(
            ECE353_RTOS_Events,
            ECE353_EVENT_SW1_PRESSED |
            ECE353_EVENT_SW2_PRESSED |
            ECE353_EVENT_SW3_PRESSED |
            ECE353_RTOS_EVENTS_IPC_DISCOVERY_RX |
            ECE353_RTOS_EVENTS_IPC_GAME_READY_RX |
            ECE353_RTOS_EVENTS_IPC_GAME_GUESS_RX |
            ECE353_RTOS_EVENTS_IPC_GAME_FEEDBACK_RX |
            ECE353_RTOS_EVENTS_IPC_GAME_TURN_END_ACK_RX |
            ECE353_RTOS_EVENTS_IPC_GAME_OVER_RX |
            ECE353_RTOS_EVENTS_IPC_GAME_RESTART_RX,
            pdTRUE,
            pdFALSE,
            0
        );

        if(hw05_read_ambient_light(&ambient_light))
        {
            hw05_theme_t new_theme = hw05_update_theme_from_light(ambient_light, theme);
            if(new_theme != theme)
            {
                theme = new_theme;
                ui_dirty = true;
            }
        }

        if((events & ECE353_EVENT_SW3_PRESSED) && (state <= HW05_STATE_WAIT_FOR_PEER_READY))
        {
            /* Ignore only very-early startup events to avoid stale/reset-edge artifacts. */
            if((xTaskGetTickCount() - startup_tick) > pdMS_TO_TICKS(500U))
            {
                high_score = HW05_EEPROM_HIGH_SCORE_UNSET;

                if(!hw05_write_high_score(high_score))
                {
                    printf("EEPROM reset failed.\n\r");
                }

                ui_dirty = true;
            }
        }

        if(state == HW05_STATE_INIT_AND_SYNC)
        {
            TickType_t now_ticks = xTaskGetTickCount();

            if(now_ticks >= next_discovery_tick)
            {
                (void)ipc_send_discovery(tx_sequence++);
                if(ipc_wait_for_ack(75U))
                {
                    sync_complete = true;
                }

                next_discovery_tick = now_ticks + pdMS_TO_TICKS(150U);
            }

            if(sync_complete)
            {
                state = HW05_STATE_SELECT_CIPHER;
                ui_dirty = true;
            }
        }

        if((events & ECE353_RTOS_EVENTS_IPC_GAME_READY_RX) != 0U)
        {
            ipc_packet_t packet = {0};

            if(hw05_consume_rx_packet(IPC_CMD_GAME_READY, &packet))
            {
                peer_ready_seen = true;

                if(!local_ready_sent)
                {
                    peer_ready_seen_before_local_ready = true;
                }

                ui_dirty = true;
            }
        }

        if((events & ECE353_RTOS_EVENTS_IPC_GAME_GUESS_RX) != 0U)
        {
            ipc_packet_t packet = {0};

            if((state == HW05_STATE_PASSIVE_WAIT_GUESS) && hw05_consume_rx_packet(IPC_CMD_GAME_GUESS, &packet))
            {
                uint8_t current_exact = 0U;
                uint8_t current_misplaced = 0U;

                hw05_copy_digits(last_guess_digits, packet.payload.game.digits);
                peer_guess_count = packet.payload.game.guess_count;
                (void)hw05_evaluate_guess(local_cipher, last_guess_digits, &current_exact, &current_misplaced);

                exact = current_exact;
                misplaced = current_misplaced;
                passive_win = (current_exact == HW05_GAME_DIGIT_COUNT);

                (void)ipc_send_game_feedback(
                    tx_sequence++,
                    current_exact,
                    current_misplaced,
                    (uint8_t)peer_guess_count,
                    passive_win
                );

                (void)ipc_wait_for_ack(250U);
                state = HW05_STATE_PASSIVE_WAIT_TURN_END_ACK;
                ui_dirty = true;
            }
        }

        if((events & ECE353_RTOS_EVENTS_IPC_GAME_FEEDBACK_RX) != 0U)
        {
            ipc_packet_t packet = {0};

            if((state == HW05_STATE_ACTIVE_WAIT_FEEDBACK) && hw05_consume_rx_packet(IPC_CMD_GAME_FEEDBACK, &packet))
            {
                exact = packet.payload.game.exact;
                misplaced = packet.payload.game.misplaced;
                local_guess_count = packet.payload.game.guess_count;

                if((packet.payload.game.flags & 0x01U) != 0U)
                {
                    if((high_score == HW05_EEPROM_HIGH_SCORE_UNSET) || (local_guess_count < high_score))
                    {
                        high_score = local_guess_count;

                        if(!hw05_write_high_score(high_score))
                        {
                            printf("EEPROM high-score update failed.\n\r");
                        }
                    }
                }

                state = HW05_STATE_ACTIVE_VIEW_FEEDBACK;

                ui_dirty = true;
            }
        }

        if((events & ECE353_RTOS_EVENTS_IPC_GAME_TURN_END_ACK_RX) != 0U)
        {
            ipc_packet_t packet = {0};

            if((state == HW05_STATE_PASSIVE_WAIT_TURN_END_ACK) && hw05_consume_rx_packet(IPC_CMD_GAME_TURN_END_ACK, &packet))
            {
                if(passive_win)
                {
                    (void)ipc_send_game_over(
                        tx_sequence++,
                        (uint8_t)local_guess_count,
                        (uint8_t)peer_guess_count,
                        false
                    );
                    (void)ipc_wait_for_ack(250U);
                    state = HW05_STATE_GAME_OVER;
                }
                else
                {
                    state = HW05_STATE_ACTIVE_BUILD_GUESS;
                    hw05_reset_round_state(entry_digits, &entry_count, &entry_cursor, exact, misplaced);
                }

                ui_dirty = true;
            }
        }

        if((events & ECE353_RTOS_EVENTS_IPC_GAME_OVER_RX) != 0U)
        {
            ipc_packet_t packet = {0};

            if((state == HW05_STATE_WAIT_GAME_OVER) && hw05_consume_rx_packet(IPC_CMD_GAME_OVER, &packet))
            {
                /* Game-over payload uses sender-local in exact, sender-peer in misplaced. */
                peer_guess_count = packet.payload.game.exact;
                local_guess_count = packet.payload.game.misplaced;
                state = HW05_STATE_GAME_OVER;
                ui_dirty = true;
            }
        }

        if((events & ECE353_RTOS_EVENTS_IPC_GAME_RESTART_RX) != 0U)
        {
            if((state == HW05_STATE_GAME_OVER) || (state == HW05_STATE_WAIT_GAME_OVER))
            {
                hw05_reset_game(
                    &state,
                    local_cipher,
                    peer_cipher,
                    entry_digits,
                    &entry_count,
                    &entry_cursor,
                    &local_guess_count,
                    &peer_guess_count,
                    &exact,
                    &misplaced,
                    &local_ready_sent,
                    &peer_ready_seen,
                    &peer_ready_seen_before_local_ready,
                    &local_goes_first
                );

                state = HW05_STATE_SELECT_CIPHER;
                sync_complete = true;
                passive_win = false;
                touch_latched = false;
                next_discovery_tick = 0U;
                ui_dirty = true;
            }
        }

        if((events & ECE353_EVENT_SW1_PRESSED) != 0U)
        {
            if((state == HW05_STATE_SELECT_CIPHER) && (entry_count == HW05_GAME_DIGIT_COUNT))
            {
                (void)hw05_copy_digits(local_cipher, entry_digits);
                (void)ipc_send_game_ready(tx_sequence++, ready_payload);
                local_ready_sent = true;
                local_goes_first = !peer_ready_seen_before_local_ready;

                if(!ipc_wait_for_ack(250U))
                {
                    printf("READY ACK timeout.\n\r");
                }

                if(peer_ready_seen)
                {
                    state = local_goes_first ? HW05_STATE_ACTIVE_BUILD_GUESS : HW05_STATE_PASSIVE_WAIT_GUESS;
                    hw05_reset_round_state(entry_digits, &entry_count, &entry_cursor, exact, misplaced);
                }
                else
                {
                    state = HW05_STATE_WAIT_FOR_PEER_READY;
                }

                ui_dirty = true;
            }
            else if((state == HW05_STATE_ACTIVE_BUILD_GUESS) && (entry_count == HW05_GAME_DIGIT_COUNT))
            {
                local_guess_count++;
                hw05_copy_digits(last_guess_digits, entry_digits);

                (void)ipc_send_game_guess(tx_sequence++, last_guess_digits, (uint8_t)local_guess_count);

                if(!ipc_wait_for_ack(250U))
                {
                    printf("GUESS ACK timeout.\n\r");
                }

                state = HW05_STATE_ACTIVE_WAIT_FEEDBACK;
                ui_dirty = true;
            }
            else if(state == HW05_STATE_ACTIVE_VIEW_FEEDBACK)
            {
                (void)ipc_send_game_turn_end_ack(tx_sequence++);
                (void)ipc_wait_for_ack(250U);

                if(exact == HW05_GAME_DIGIT_COUNT)
                {
                    state = HW05_STATE_WAIT_GAME_OVER;
                }
                else
                {
                    state = HW05_STATE_PASSIVE_WAIT_GUESS;
                    hw05_reset_round_state(entry_digits, &entry_count, &entry_cursor, exact, misplaced);
                }

                ui_dirty = true;
            }
        }

        if((events & ECE353_EVENT_SW2_PRESSED) != 0U)
        {
            if((state == HW05_STATE_SELECT_CIPHER) || (state == HW05_STATE_ACTIVE_BUILD_GUESS))
            {
                if(entry_count > 0U)
                {
                    entry_count--;
                    entry_cursor = entry_count;
                    entry_digits[entry_count] = HW05_BLANK_DIGIT;
                    ui_dirty = true;
                }
            }
            else if((state == HW05_STATE_GAME_OVER) || (state == HW05_STATE_WAIT_GAME_OVER))
            {
                /*
                 * One SW2 press should restart both boards.
                 * Broadcast restart a few times for link robustness, then reset locally.
                 */
                for(uint8_t attempt = 0U; attempt < 3U; attempt++)
                {
                    (void)ipc_send_game_restart(tx_sequence++, 0U);
                    (void)ipc_wait_for_ack(120U);
                }

                hw05_reset_game(
                    &state,
                    local_cipher,
                    peer_cipher,
                    entry_digits,
                    &entry_count,
                    &entry_cursor,
                    &local_guess_count,
                    &peer_guess_count,
                    &exact,
                    &misplaced,
                    &local_ready_sent,
                    &peer_ready_seen,
                    &peer_ready_seen_before_local_ready,
                    &local_goes_first
                );

                state = HW05_STATE_SELECT_CIPHER;
                sync_complete = true;
                passive_win = false;
                touch_latched = false;
                next_discovery_tick = 0U;
                ui_dirty = true;
            }
        }

        if((state == HW05_STATE_WAIT_FOR_PEER_READY) && peer_ready_seen)
        {
            state = local_goes_first ? HW05_STATE_ACTIVE_BUILD_GUESS : HW05_STATE_PASSIVE_WAIT_GUESS;
            hw05_reset_round_state(entry_digits, &entry_count, &entry_cursor, exact, misplaced);
            ui_dirty = true;
        }

        if((state == HW05_STATE_SELECT_CIPHER) || (state == HW05_STATE_ACTIVE_BUILD_GUESS))
        {
            uint16_t touch_x = 0U;
            uint16_t touch_y = 0U;
            uint8_t digit = 0U;
            bool touch_detected = false;

            touch_detected = hw05_read_touch(&touch_x, &touch_y) && hw05_touch_to_digit(touch_x, touch_y, &digit);

            if((entry_count < HW05_GAME_DIGIT_COUNT) && touch_detected && !touch_latched)
            {
                entry_digits[entry_count] = digit;
                entry_count++;
                entry_cursor = entry_count;
                touch_latched = true;
                ui_dirty = true;
            }

            if(!touch_detected)
            {
                touch_latched = false;
            }
        }
        else
        {
            touch_latched = false;
        }

        if(ui_dirty)
        {
            const uint8_t *render_digits = entry_digits;

            if((state == HW05_STATE_PASSIVE_WAIT_TURN_END_ACK) || (state == HW05_STATE_ACTIVE_WAIT_FEEDBACK))
            {
                render_digits = last_guess_digits;
            }

            if(state == HW05_STATE_GAME_OVER)
            {
                hw05_render_current_state(
                    state,
                    theme,
                    render_digits,
                    0U,
                    0U,
                    high_score,
                    local_guess_count,
                    peer_guess_count,
                    exact,
                    misplaced,
                    passive_win
                );
            }
            else if(state == HW05_STATE_WAIT_GAME_OVER)
            {
                hw05_render_current_state(
                    state,
                    theme,
                    render_digits,
                    HW05_GAME_DIGIT_COUNT,
                    HW05_GAME_DIGIT_COUNT,
                    high_score,
                    local_guess_count,
                    peer_guess_count,
                    exact,
                    misplaced,
                    passive_win
                );
            }
            else
            {
                hw05_render_current_state(
                    state,
                    theme,
                    render_digits,
                    entry_count,
                    entry_cursor,
                    high_score,
                    local_guess_count,
                    peer_guess_count,
                    exact,
                    misplaced,
                    passive_win
                );
            }

            ui_dirty = false;
        }

        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

/*****************************************************************************/
/* Application Code                                                          */
/*****************************************************************************/
void app_main(void)
{
    ECE353_RTOS_Events = xEventGroupCreate();
    if(ECE353_RTOS_Events == NULL)
    {
        hw05_die("Failed to create RTOS event group!");
    }

    I2C_Monarch_Semaphore = xSemaphoreCreateBinary();
    if(I2C_Monarch_Semaphore == NULL)
    {
        hw05_die("Failed to create I2C semaphore!");
    }
    xSemaphoreGive(I2C_Monarch_Semaphore);

    SPI_Monarch_Semaphore = xSemaphoreCreateBinary();
    if(SPI_Monarch_Semaphore == NULL)
    {
        hw05_die("Failed to create SPI semaphore!");
    }
    xSemaphoreGive(SPI_Monarch_Semaphore);

    xQueue_Request_LCD = xQueueCreate(10, sizeof(lcd_msg_request_t));
    if(xQueue_Request_LCD == NULL)
    {
        hw05_die("Failed to create LCD queue!");
    }

    Queue_Cap_Touch_Responses = xQueueCreate(1, sizeof(device_response_msg_t));
    if(Queue_Cap_Touch_Responses == NULL)
    {
        hw05_die("Failed to create cap touch response queue!");
    }

    Queue_EEPROM_Responses = xQueueCreate(1, sizeof(device_response_msg_t));
    if(Queue_EEPROM_Responses == NULL)
    {
        hw05_die("Failed to create EEPROM response queue!");
    }

    Queue_Light_Sensor_Responses = xQueueCreate(1, sizeof(device_response_msg_t));
    if(Queue_Light_Sensor_Responses == NULL)
    {
        hw05_die("Failed to create light sensor response queue!");
    }

    if(!task_console_init())
    {
        hw05_die("Console task initialization failed!");
    }

    if(!task_button_init())
    {
        hw05_die("Button task initialization failed!");
    }

    if(!task_lcd_resources_init(xQueue_Request_LCD))
    {
        hw05_die("LCD task initialization failed!");
    }

    if(!task_eeprom_resources_init(&SPI_Monarch_Semaphore, SPI_Monarch_Obj, PIN_SPI_EEPROM_CS))
    {
        hw05_die("EEPROM task initialization failed!");
    }

    if(!task_cap_touch_resources_init(NULL, I2C_Monarch_Semaphore, I2C_Monarch_Obj, NC))
    {
        hw05_die("Cap touch task initialization failed!");
    }

    if(!task_light_sensor_resources_init(&I2C_Monarch_Semaphore, I2C_Monarch_Obj))
    {
        hw05_die("Light sensor task initialization failed!");
    }

    if(!task_ipc_init())
    {
        hw05_die("IPC task initialization failed!");
    }

    if(xTaskCreate(
        task_hw05_system_control,
        "HW05 System Control",
        TASK_SYSTEM_CONTROL_STACK_SIZE,
        NULL,
        TASK_SYSTEM_CONTROL_PRIORITY,
        NULL) != pdPASS)
    {
        hw05_die("System control task creation failed!");
    }

    vTaskStartScheduler();

    while(1)
    {
    }
}
#endif