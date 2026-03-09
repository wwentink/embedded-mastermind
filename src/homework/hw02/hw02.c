 /**
 * @file hw02.c
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2025-10-08
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include "hw02.h"

#if defined(HW02)

char APP_DESCRIPTION[] = "ECE353 S26 HW02";

/*****************************************************************************/
/* Global Variables                                                          */
/*****************************************************************************/
/* Event group for system events */
EventGroupHandle_t ECE353_RTOS_Events = NULL;

/* Queue for joystick device requests */
QueueHandle_t Queue_Requests_Joystick = NULL;

/* Queue for LCD requests */
QueueHandle_t Queue_LCD_Request = NULL;

/* Color definitions */
#define CYPHER_BG_COLOR     LCD_COLOR_RED
#define CYPHER_FG_COLOR     LCD_COLOR_BLACK
#define CYPHER_INACTIVE_BG  LCD_COLOR_BLACK
#define CYPHER_INACTIVE_FG  LCD_COLOR_RED
#define DATA_ACTIVE_BG      LCD_COLOR_GREEN
#define DATA_ACTIVE_FG      LCD_COLOR_BLACK
#define DATA_INACTIVE_BG    LCD_COLOR_BLACK
#define DATA_INACTIVE_FG    LCD_COLOR_GREEN

/*****************************************************************************/
/* Function Definitions                                                      */
/*****************************************************************************/

/**
 * @brief
 * Send a tile message to the LCD task (normal)
 */
static void send_tile_to_lcd(uint8_t row, uint8_t col, uint8_t digit, 
                             uint16_t fg_color, uint16_t bg_color)
{
    lcd_msg_request_t lcd_request;
    lcd_msg_t msg;
    
    msg.command = LCD_CMD_DRAW_TILE;
    msg.payload.tile.row = row;
    msg.payload.tile.col = col;
    msg.payload.tile.number = digit;
    msg.payload.tile.color_fg = fg_color;
    msg.payload.tile.color_bg = bg_color;
    
    lcd_request.msg = msg;
    lcd_request.return_queue = NULL;
    
    xQueueSend(Queue_LCD_Request, &lcd_request, pdMS_TO_TICKS(100));
}

/**
 * @brief
 * Send a tile message to the LCD task (inverted - for active tiles)
 */
static void send_tile_inverted_to_lcd(uint8_t row, uint8_t col, uint8_t digit, 
                                      uint16_t fg_color, uint16_t bg_color)
{
    lcd_msg_request_t lcd_request;
    lcd_msg_t msg;
    
    msg.command = LCD_CMD_DRAW_TILE_INVERTED;
    msg.payload.tile.row = row;
    msg.payload.tile.col = col;
    msg.payload.tile.number = digit;
    msg.payload.tile.color_fg = fg_color;
    msg.payload.tile.color_bg = bg_color;
    
    lcd_request.msg = msg;
    lcd_request.return_queue = NULL;
    
    xQueueSend(Queue_LCD_Request, &lcd_request, pdMS_TO_TICKS(100));
}

/**
 * @brief
 * Send a message string to the LCD task
 */
static void send_message_to_lcd(const char *message)
{
    lcd_msg_request_t lcd_request;
    lcd_msg_t msg;
    
    msg.command = LCD_CMD_PRINT_MESSAGE;
    strncpy(msg.payload.message, message, sizeof(msg.payload.message) - 1);
    msg.payload.message[sizeof(msg.payload.message) - 1] = '\0';
    
    lcd_request.msg = msg;
    lcd_request.return_queue = NULL;
    
    xQueueSend(Queue_LCD_Request, &lcd_request, pdMS_TO_TICKS(100));
}

/**
 * @brief
 * Initialize the game board: draw all tiles
 */
static void initialize_board(uint8_t cypher_col, uint8_t input_col, const uint8_t input_tile_digits[8])
{
    /* Clear the screen */
    lcd_msg_request_t lcd_request;
    lcd_msg_t msg;
    msg.command = LCD_CMD_CLEAR_SCREEN;
    lcd_request.msg = msg;
    lcd_request.return_queue = NULL;
    xQueueSend(Queue_LCD_Request, &lcd_request, pdMS_TO_TICKS(100));
    
    vTaskDelay(pdMS_TO_TICKS(100));
    
    /* Draw "Select Your Cypher!" message */
    send_message_to_lcd("Select Your Cypher!");
    
    vTaskDelay(pdMS_TO_TICKS(50));
    
    /* Draw cypher tiles (row 0) */
    for (uint8_t col = 0; col < 4; col++)
    {
        if (col == cypher_col)
        {
            send_tile_to_lcd(LCD_TILE_ROW_CYPHER, col, 0, CYPHER_FG_COLOR, CYPHER_BG_COLOR);
        }
        else
        {
            send_tile_to_lcd(LCD_TILE_ROW_CYPHER, col, 0, CYPHER_INACTIVE_FG, CYPHER_INACTIVE_BG);
        }
    }
    
    vTaskDelay(pdMS_TO_TICKS(50));
    
    /* Draw data entry tiles (row 1, columns 0-7) with exactly one active tile */
    for (uint8_t i = 0; i < 8; i++)
    {
        uint8_t lcd_row = (i < 4) ? LCD_TILE_ROW_NUM_0_3 : LCD_TILE_ROW_NUM_4_7;
        uint8_t lcd_col = i % 4;
        bool is_active = (i == input_col);
        uint16_t fg = is_active ? DATA_ACTIVE_FG : DATA_INACTIVE_FG;
        uint16_t bg = is_active ? DATA_ACTIVE_BG : DATA_INACTIVE_BG;

        send_tile_to_lcd(lcd_row, lcd_col, input_tile_digits[i], fg, bg);
    }
    
    vTaskDelay(pdMS_TO_TICKS(50));
}

/**
 * @brief
 * HW02 System Control Task
 * Implements the state machine for cypher selection game
 */
void task_hw02_system_control(void *pvParameters)
{
    (void)pvParameters; // Unused parameter

    uint8_t active_cypher_col = 0;
    uint8_t active_data_col = 0;      /* 0-7: which data tile is active */
    uint8_t cypher[4] = {0, 0, 0, 0};
    uint8_t input_tile_digits[8] = {0, 1, 2, 3, 4, 5, 6, 7};
    bool cypher_complete = false;
    bool joystick_must_center = false;
    const EventBits_t input_events =
        EVENT_JOYSTICK_UP | EVENT_JOYSTICK_DOWN | EVENT_JOYSTICK_LEFT | EVENT_JOYSTICK_RIGHT |
        EVENT_JOYSTICK_CENTER | EVENT_BUTTON_SW1 | EVENT_BUTTON_SW2 | EVENT_BUTTON_SW3;
    
    /* Initialize the board */
    initialize_board(active_cypher_col, active_data_col, input_tile_digits);

    while (1)
    {
        /* Keep the task alive and discard all future input once complete. */
        if (cypher_complete)
        {
            (void)xEventGroupWaitBits(
                ECE353_RTOS_Events,
                input_events,
                pdTRUE,
                pdFALSE,
                portMAX_DELAY
            );
            continue;
        }

        EventBits_t event_bits = xEventGroupWaitBits(
            ECE353_RTOS_Events,
            input_events,
            pdTRUE,
            pdFALSE,
            portMAX_DELAY
        );

        /* Handle SW1 immediately so button input is never blocked by joystick center gating */
        if (event_bits & EVENT_BUTTON_SW1)
        {
            uint8_t old_cypher_col = active_cypher_col;
            uint8_t selected_digit = input_tile_digits[active_data_col];

            /* Write the selected digit into the current cypher slot (filled style: red on black) */
            cypher[old_cypher_col] = selected_digit;
            send_tile_to_lcd(LCD_TILE_ROW_CYPHER, old_cypher_col, selected_digit,
                            CYPHER_INACTIVE_FG, CYPHER_INACTIVE_BG);

            active_cypher_col++;

            if (active_cypher_col >= 4)
            {
                cypher_complete = true;
                continue;
            }

            send_tile_to_lcd(LCD_TILE_ROW_CYPHER, active_cypher_col, cypher[active_cypher_col],
                                      CYPHER_FG_COLOR, CYPHER_BG_COLOR);
        }

        /* Handle joystick return to center first */
        if (event_bits & EVENT_JOYSTICK_CENTER)
        {
            joystick_must_center = false;
            continue;  /* Wait for next movement */
        }

        /* If joystick must return to center, ignore movement events */
        if (joystick_must_center)
        {
            continue;
        }

        /* Handle joystick movements */
        if (event_bits & EVENT_JOYSTICK_LEFT)
        {
            if (active_data_col > 0)
            {
                uint8_t prev_col = active_data_col;
                active_data_col--;

                uint8_t prev_lcd_row = (prev_col < 4) ? LCD_TILE_ROW_NUM_0_3 : LCD_TILE_ROW_NUM_4_7;
                uint8_t new_lcd_row = (active_data_col < 4) ? LCD_TILE_ROW_NUM_0_3 : LCD_TILE_ROW_NUM_4_7;
                uint8_t prev_lcd_col = prev_col % 4;
                uint8_t new_lcd_col = active_data_col % 4;

                /* Redraw previous tile (inactive) and new tile (active) */
                send_tile_to_lcd(prev_lcd_row, prev_lcd_col, input_tile_digits[prev_col], DATA_INACTIVE_FG, DATA_INACTIVE_BG);
                send_tile_to_lcd(new_lcd_row, new_lcd_col, input_tile_digits[active_data_col], DATA_ACTIVE_FG, DATA_ACTIVE_BG);
            }
            joystick_must_center = true;
        }
        else if (event_bits & EVENT_JOYSTICK_RIGHT)
        {
            if (active_data_col < 7)
            {
                uint8_t prev_col = active_data_col;
                active_data_col++;

                uint8_t prev_lcd_row = (prev_col < 4) ? LCD_TILE_ROW_NUM_0_3 : LCD_TILE_ROW_NUM_4_7;
                uint8_t new_lcd_row = (active_data_col < 4) ? LCD_TILE_ROW_NUM_0_3 : LCD_TILE_ROW_NUM_4_7;
                uint8_t prev_lcd_col = prev_col % 4;
                uint8_t new_lcd_col = active_data_col % 4;

                /* Redraw previous tile (inactive) and new tile (active) */
                send_tile_to_lcd(prev_lcd_row, prev_lcd_col, input_tile_digits[prev_col], DATA_INACTIVE_FG, DATA_INACTIVE_BG);
                send_tile_to_lcd(new_lcd_row, new_lcd_col, input_tile_digits[active_data_col], DATA_ACTIVE_FG, DATA_ACTIVE_BG);
            }
            joystick_must_center = true;
        }
        else if (event_bits & EVENT_JOYSTICK_UP)
        {
            if (active_data_col >= 4)
            {
                uint8_t prev_col = active_data_col;
                active_data_col -= 4;

                send_tile_to_lcd(LCD_TILE_ROW_NUM_4_7, prev_col % 4, input_tile_digits[prev_col], DATA_INACTIVE_FG, DATA_INACTIVE_BG);
                send_tile_to_lcd(LCD_TILE_ROW_NUM_0_3, active_data_col, input_tile_digits[active_data_col], 
                                DATA_ACTIVE_FG, DATA_ACTIVE_BG);
            }
            joystick_must_center = true;
        }
        else if (event_bits & EVENT_JOYSTICK_DOWN)
        {
            if (active_data_col <= 3)
            {
                uint8_t prev_col = active_data_col;
                active_data_col += 4;

                send_tile_to_lcd(LCD_TILE_ROW_NUM_0_3, prev_col, input_tile_digits[prev_col], DATA_INACTIVE_FG, DATA_INACTIVE_BG);
                send_tile_to_lcd(LCD_TILE_ROW_NUM_4_7, active_data_col % 4, input_tile_digits[active_data_col], 
                                DATA_ACTIVE_FG, DATA_ACTIVE_BG);
            }
            joystick_must_center = true;
        }
    }
}


/*************************************************
 * @brief
 * This function will initialize all of the hardware resources for
 * the HW02 assignment
 ************************************************/
void app_init_hw(void)
{
    cy_rslt_t rslt;

    console_init();
    // Set text color to black
    printf("\x1b[30m");
    printf("\x1b[2J\x1b[;H");
    printf("**************************************************\n\r");
    printf("* %s\n\r", APP_DESCRIPTION);
    printf("* Date: %s\n\r", __DATE__);
    printf("* Time: %s\n\r", __TIME__);
    printf("* Name:%s\n\r", NAME);
    printf("**************************************************\n\r");

    rslt = lcd_initialize();
    if (rslt != CY_RSLT_SUCCESS)
    {
        printf("LCD initialization failed!\n\r");
        for(int i = 0; i < 100000; i++) {}
        CY_ASSERT(0);
    }

    /* Initialize the joystick */
    rslt = joystick_init();
    if (rslt != CY_RSLT_SUCCESS)
    {
        printf("Joystick initialization failed!\n\r");
        for(int i = 0; i < 100000; i++) {}
        CY_ASSERT(0);
    }
}

/*****************************************************************************/
/* Application Code                                                          */
/*****************************************************************************/
/**
 * @brief
 * This function implements the behavioral requirements for HW02
 */
void app_main(void)
{
    printf("Starting HW02 App Main...\n\r");
    
    /* Create the event group for RTOS events */
    ECE353_RTOS_Events = xEventGroupCreate();
    if (ECE353_RTOS_Events == NULL)
    {
        printf("Failed to create event group\n\r");
        for(int i = 0; i < 100000; i++) {}
        CY_ASSERT(0);
    }
    printf("Event group created\n\r");

    /* Create the FreeRTOS queues */
    Queue_Requests_Joystick = xQueueCreate(10, sizeof(device_request_msg_t));
    if (Queue_Requests_Joystick == NULL)
    {
        printf("Failed to create joystick queue\n\r");
        for(int i = 0; i < 100000; i++) {}
        CY_ASSERT(0);
    }
    printf("Joystick queue created\n\r");

    Queue_LCD_Request = xQueueCreate(10, sizeof(lcd_msg_request_t));
    if (Queue_LCD_Request == NULL)
    {
        printf("Failed to create LCD queue\n\r");
        for(int i = 0; i < 100000; i++) {}
        CY_ASSERT(0);
    }
    printf("LCD queue created\n\r");

    /* Initialize and create the LCD task */
    if (!task_lcd_resources_init(Queue_LCD_Request))
    {
        printf("Failed to initialize LCD task\n\r");
        for(int i = 0; i < 100000; i++) {}
        CY_ASSERT(0);
    }
    printf("LCD task created\n\r");

    /* Initialize and create the button task */
    if (!task_button_init())
    {
        printf("Failed to initialize button task\n\r");
        for(int i = 0; i < 100000; i++) {}
        CY_ASSERT(0);
    }
    printf("Button task created\n\r");

    /* Initialize and create the device gatekeeper task (BEFORE joystick task!) */
    if (!task_device_gatekeeper_init())
    {
        printf("Failed to initialize device gatekeeper task\n\r");
        for(int i = 0; i < 100000; i++) {}
        CY_ASSERT(0);
    }
    printf("Device gatekeeper task created\n\r");

    /* Initialize and create the joystick task */
    if (!task_joystick_init())
    {
        printf("Failed to initialize joystick task\n\r");
        for(int i = 0; i < 100000; i++) {}
        CY_ASSERT(0);
    }
    printf("Joystick task created\n\r");

    /* Create the system control task */
    BaseType_t result = xTaskCreate(
        task_hw02_system_control,
        "System Control Task",
        TASK_SYSTEM_CONTROL_STACK_SIZE,
        NULL,
        TASK_SYSTEM_CONTROL_PRIORITY,
        NULL
    );
    if (result != pdPASS)
    {
        printf("Failed to create system control task\n\r");
        for(int i = 0; i < 100000; i++) {}
        CY_ASSERT(0);
    }
    printf("System control task created\n\r");

    /* Start the scheduler */
    printf("Starting scheduler...\n\r");
    vTaskStartScheduler();

    /* Will never reach this loop once the scheduler starts */
    while (1)
    {
    }
}
#endif