/**
 * @file task_lcd.c
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2025-08-18
 * 
 * @copyright Copyright (c) 2025
 * 
 */

 #include "task_lcd.h"
#include "drivers.h"
#include "master_mind_lib.h"

#if defined(ECE353_FREERTOS)

/* FreeRTOS Queue for LCD messages */
static QueueHandle_t Queue_Requests = NULL;

/* LCD Task */
void task_lcd(void *pvParameters)
{
    (void)pvParameters; // Unused parameter

    lcd_msg_request_t lcd_request;

    while(1)
    {
        // Wait for a message from the queue
        if (xQueueReceive(Queue_Requests, &lcd_request, portMAX_DELAY))
        {
            // Process the message based on command type
            switch(lcd_request.msg.command)
            {
                case LCD_CMD_CLEAR_SCREEN:
                    lcd_clear_screen(LCD_COLOR_BLACK);
                    break;
                    
                case LCD_CMD_PRINT_MESSAGE:
                {
                    // Print the message to the LCD using font drawing
                    const char* str = lcd_request.msg.payload.message;
                    int cx = 5;  // Left margin
                    int cy = Consolas_20ptFontInfo.height / 2 + 10;  // Top margin
                    
                    // Draw each character
                    for (int i = 0; str[i] != '\0'; i++)
                    {
                        char c = str[i];
                        
                        // Handle space and printable characters
                        if (c == ' ')
                        {
                            cx += Consolas_20ptFontInfo.space_width;
                        }
                        else if (c >= Consolas_20ptFontInfo.start_char && c <= Consolas_20ptFontInfo.end_char)
                        {
                            int char_index = c - Consolas_20ptFontInfo.start_char;
                            int char_width = Consolas_20ptFontInfo.char_info[char_index].width;
                            
                            lcd_draw_image(
                                cx + char_width / 2,
                                cy,
                                char_width,
                                Consolas_20ptFontInfo.height,
                                Consolas_20ptBitmaps + Consolas_20ptFontInfo.char_info[char_index].offset,
                                LCD_COLOR_WHITE,
                                LCD_COLOR_BLACK,
                                true
                            );
                            
                            cx += char_width;
                        }
                    }
                    break;
                }
                    
                case LCD_CMD_DRAW_TILE:
                    // TODO: Draw a tile on the LCD
                    break;
                    
                case LCD_CMD_DRAW_TILE_INVERTED:
                    // TODO: Draw an inverted tile on the LCD
                    break;
                    
                default:
                    break;
            }
        }
    }
}

/* LCD Task Initialization */
bool task_lcd_resources_init(QueueHandle_t queue_request){

    BaseType_t result;

    if (queue_request == NULL)
    {
        return false;
    }
    Queue_Requests = queue_request;

    /* Create the LCD Task */
    result= xTaskCreate(
        task_lcd,                       // Task function
        "LCD Task",                     // Task name
        TASK_LCD_STACK_SIZE,            // Stack size
        NULL,                           // Task parameters
        TASK_LCD_PRIORITY,              // Task priority
        NULL                            // Task handle
    );

    if(result != pdPASS)
    {
        return false;
    }   

    return true;
}
#endif