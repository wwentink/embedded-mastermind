 /**
 * @file hw05.c
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2025-10-08
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include "hw05.h"

#if defined(HW05)
#include "task_buttons.h"
#include "task_ipc.h"
#include "task_light_sensor.h"


char APP_DESCRIPTION[] = "ECE353 S26 HW05";

/*****************************************************************************/
/* Global Variables                                                          */
/*****************************************************************************/
cyhal_i2c_t *I2C_Monarch_Obj;
cyhal_spi_t *SPI_Monarch_Obj;
SemaphoreHandle_t I2C_Monarch_Semaphore;
SemaphoreHandle_t SPI_Monarch_Semaphore;
EventGroupHandle_t ECE353_RTOS_Events = NULL;

/* Queue for joystick device requests */
QueueHandle_t Queue_Requests_Joystick = NULL;

/* Queue for LCD requests */
QueueHandle_t Queue_LCD_Request = NULL;

// state machine variables
typedef enum {
    IDLE,
    SYNC,
    PICK_CYPHER,
    GUESSING_1,
    GUESSING_2,
    FEEDBACK,
    WINNER
} hw05_state_t;

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

/* This function will be used to discover other board. This function should not
 * return until the discovery is complete.  The discovery is complete when we receive
 * a discovery message from the other board OR we send a discovery message that is 
 * Acked by the other board */
void discover_board(uint16_t *sequence_num)
{
    bool discovery_complete = false;

    /* Clear any stale IPC events/ACK state before we begin discovery retries. */
    xEventGroupClearBits(
        ECE353_RTOS_Events,
        ECE353_RTOS_EVENTS_IPC_ACK_RECEIVED | ECE353_RTOS_EVENTS_IPC_DISCOVERY_RX
    );

    taskENTER_CRITICAL();
    IPC_Ack_Sequence_Valid = false;
    taskEXIT_CRITICAL();
    
    while(discovery_complete == false)
    {
        EventBits_t events;

        ipc_send_discovery((*sequence_num)++);

        events = xEventGroupWaitBits(
            ECE353_RTOS_Events,
            ECE353_RTOS_EVENTS_IPC_ACK_RECEIVED | ECE353_RTOS_EVENTS_IPC_DISCOVERY_RX,
            pdTRUE,
            pdFALSE,
            pdMS_TO_TICKS(250)
        );

        if(events & (ECE353_RTOS_EVENTS_IPC_ACK_RECEIVED | ECE353_RTOS_EVENTS_IPC_DISCOVERY_RX))
        {
            discovery_complete = true;

            if(events & ECE353_RTOS_EVENTS_IPC_DISCOVERY_RX)
            {
                printf("Discovery message received from other board!\n\r");
            }

            if(events & ECE353_RTOS_EVENTS_IPC_ACK_RECEIVED)
            {
                printf("ACK received for discovery message!\n\r");
            }
        }
        else
        {
            printf("No response to discovery message. Retrying...\n\r");
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

/*****************************************************************************/
/* System control state machine code                                         */
/*****************************************************************************/
/**
 * @brief
 * This function implements the behavioral requirements for the ICE
 */

void task_hw05_system_control(void *pvParameters) {
        (void)pvParameters; // Unused parameter
    
        /* Implement the state machine for HW05 here. You may create helper functions
        * to break up the code if you wish, but all state machine logic should be contained
        * in this function. */

        hw05_state_t current_state = IDLE;

        while (1) {
            switch (current_state) {
                case IDLE:
                    // Initialize hardware resources
                    app_init_hw();
                    current_state = SYNC;
                    break;
                case SYNC:
                    // Perform board discovery
                    uint16_t sequence_num = 0;
                    discover_board(&sequence_num);
                    current_state = PICK_CYPHER;
                    break;
                case PICK_CYPHER:
                    // Implement cypher selection logic
                    break;
                case GUESSING_1:
                    // Implement first guessing logic
                    break;
                case GUESSING_2:
                    // Implement second guessing logic
                    break;
                case FEEDBACK:
                    // Implement feedback logic
                    break;
                case WINNER:
                    // Implement winner logic
                    break;
                default:
                    // Handle unexpected state
                    break;
            }
        }
}   

/*************************************************
 * @brief
 * This function will initialize all of the hardware resources for
 * the ICE
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

    // Initialize the LCD
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

    // Initialize the buttons
    rslt = buttons_init_gpio();
    if(rslt != CY_RSLT_SUCCESS)
    {
        printf("Button GPIO initialization failed!\n\r");
        for(int i = 0; i < 10000; i++);
        CY_ASSERT(0);
    }

    /* Initialize the I2C interface */
    I2C_Monarch_Obj = i2c_init(PIN_I2C_SDA, PIN_I2C_SCL);
    if( I2C_Monarch_Obj == NULL)
    {
        printf("I2C Initialization Failed!\n\r");
        for(int i = 0; i < 100000; i++) {}
        CY_ASSERT(0);
    }

    /* Initialize SPI Interface */
    SPI_Monarch_Obj = spi_init(PIN_SPI_MOSI, PIN_SPI_MISO, PIN_SPI_CLK);
    if( SPI_Monarch_Obj == NULL)
    {
        printf("SPI Initialization Failed!\n\r");
        for(int i = 0; i < 100000; i++) {}
        CY_ASSERT(0);
    }

    /* Initialize the EEPROM chip select pin. */
    cyhal_gpio_init(PIN_SPI_EEPROM_CS, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, 1);
}



/*****************************************************************************/
/* Application Code                                                          */
/*****************************************************************************/
/**
 * @brief
 * This function implements the behavioral requirements for the ICE
 */
void app_main(void)
{
    bool rslt; 
    
    /* Start any other tasks required to complete this homework */

    //Start eeprom task
    rslt = task_eeprom_resources_init(&SPI_Monarch_Semaphore, SPI_Monarch_Obj, PIN_SPI_EEPROM_CS);
    if (!rslt)
    {
        printf("EEPROM Task resource initialization failed!\n\r");
        for(int i = 0; i < 100000; i++) {}
        CY_ASSERT(0);
    }

    //Start cap touch task
    rslt = task_cap_touch_resources_init(
        NULL,
        I2C_Monarch_Semaphore,
        I2C_Monarch_Obj,
        NC
    );
    if (!rslt)
    {
        printf("Cap Touch Task resource initialization failed!\n\r");
        for(int i = 0; i < 100000; i++) {}
        CY_ASSERT(0);
    }

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
        task_hw05_system_control, //HW05 system control task
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